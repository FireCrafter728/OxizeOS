#include <Drivers/MMD/phys.hpp>

using namespace SysKrnl64::MMD;

MemoryAllocErrors PhysAlloc::Initialize(SystemTable* System)
{
    // Load and verify data
    if(!System) {
        printf("[SYSKRNL64] [PHYS ALLOC] [ERROR]: Invalid System table ptr specified\r\n");
        return MMD_INVALID_PARAMETER;
    }

    this->bitmap = reinterpret_cast<uint8_t*>(System->memLayout.PhysAllocBitmapAddr);
    this->bitmapPageCount = System->memLayout.PhysAllocBitmapPages;
    this->lastFreeBlockOffset = 0;
    
    if(!this->bitmap || this->bitmapPageCount == 0)
    {
        printf("[SYSKRNL64] [PHYS ALLOC] [ERROR]: Invalid System table data for physical allocator\r\n");
        return MMD_INVALID_INIT_DATA;
    }

    // Clear the bitmap by setting it all to 0 as initial state

    size_t bitmapSizeBytes = this->bitmapPageCount * PAGE_SIZE;

    memset(this->bitmap, 0, bitmapSizeBytes);

    // Build memory ranges describing memory

    // MEMTYPE_USABLE marks free blocks
    // MEMTYPE_SOFTWARE_RESERVED marks used blocks
    // Other entries do not describe physical DRAM

    MemoryRegion* memRegions = System->memTable.regions;

    uint32_t nextRangeIndex = 0;
    uintptr_t currentBitmapIndex = 0;

    for(size_t i = 0; i < System->memTable.regionCount; i++)
    {
        MemoryRegion* region = &memRegions[i];
        if(region->type != MEMTYPE_USABLE && region->type != MEMTYPE_SOFTWARE_RESERVED) continue;

        MemoryRange* range = &memoryRanges[nextRangeIndex];
        range->start = region->phys;
        range->length = region->length;
        range->bitmapIndexBase = currentBitmapIndex;

        // Mark the software reserved region as used
        if(region->type == MEMTYPE_SOFTWARE_RESERVED)
        {
            size_t pageCount = BLOCK_COUNT(range->length);

            for(size_t p = 0; p < pageCount; p++)
            {
                size_t bitIndex = range->bitmapIndexBase + p;
                this->bitmap[bitIndex >> 3] |= (1U << (bitIndex & 7));
            }
        }

        currentBitmapIndex += region->length / PAGE_SIZE;
        nextRangeIndex++;
    }

    return MMD_SUCCESS;
}

std::expected<uintptr_t, MemoryAllocErrors> PhysAlloc::AllocContiguousBlocks(size_t blockCount)
{
    if(blockCount == 0) return 0; // return success with a nullptr

    const uint64_t totalBlocks = static_cast<uint64_t>(bitmapPageCount) * PAGE_SIZE * 8ULL;
    if(blockCount > totalBlocks) return std::unexpected<MemoryAllocErrors>(MMD_OUT_OF_MEMORY);

    auto isFree = [&](uint64_t block) -> bool
    {
        const uint64_t* bm = reinterpret_cast<const uint64_t*>(bitmap);
        return ((bm[block >> 6] >> (block & 63)) & 1ULL) == 0;
    };

    auto markAllocated = [&](uint64_t start)
    {
        uint64_t* bm = reinterpret_cast<uint64_t*>(bitmap);
        for(size_t i = 0; i < blockCount; i++)
        {
            const uint64_t block = start + i;
            bm[block >> 6] |= 1ULL << (block & 63);
        }
    };

    auto findRun = [&](uint64_t begin, uint64_t end, uint64_t& outStart) -> bool
    {
        uint64_t runStart = 0, runLen = 0;

        for(uint64_t i = begin; i < end; i++)
        {
            if(isFree(i))
            {
                if(runLen == 0) runStart = i;

                if(++runLen == blockCount)
                {
                    outStart = runStart;
                    return true;
                }
            }
            else runLen = 0;
        }

        return false;
    };

    uint64_t start = 0;
    uint64_t hint = lastFreeBlockOffset < totalBlocks ? lastFreeBlockOffset : 0;

    if(findRun(hint, totalBlocks, start) || (hint != 0 && findRun(0, hint, start)))
    {
        markAllocated(start);
        lastFreeBlockOffset = (start + blockCount) % totalBlocks;
        return BitOffsetToAddr(start); // Convert block index in bitmap to an actual address
    }

    return std::unexpected<MemoryAllocErrors>(MMD_OUT_OF_MEMORY);
}

MemoryAllocErrors PhysAlloc::AllocSparseBlocksToContiguousVirtualRange(size_t blockCount, uintptr_t virt, uint64_t pageFlags)
{
    if(blockCount == 0) return MMD_SUCCESS;
    
    const uint64_t totalBlocks = static_cast<uint64_t>(bitmapPageCount) * PAGE_SIZE * 8ULL;
    if(blockCount > totalBlocks) return MMD_OUT_OF_MEMORY;

    const uint64_t searchStartBlock = lastFreeBlockOffset < totalBlocks ? lastFreeBlockOffset : 0;
    const uint64_t searchStartQword = searchStartBlock >> 6;
    const uint64_t searchStartBit = searchStartBlock & 63;
    const size_t totalQwords = totalBlocks >> 6;

    const uint64_t* bm = reinterpret_cast<const uint64_t*>(bitmap);
    uint64_t* bm64 = reinterpret_cast<uint64_t*>(bitmap);

    auto markAllocatedRange = [&](uintptr_t startBit, size_t count)
    {
        uint64_t bit = startBit;
        size_t left = count;

        while(left > 0)
        {
            uint64_t q = bit >> 6;
            uint64_t off = bit & 63;
            size_t take = left < (64 - off) ? left : (64 - off);
            uint64_t mask = take == 64 ? UINT64_MAX : ((1ULL << take) - 1ULL);
            bm64[q] |= (mask << off);
            bit += take;
            left -= take;
        }
    };

    uintptr_t currVirt = virt, runVirt = 0;
    uint64_t runStartBit = 0;
    size_t runBlocks = 0, blocksLeft = blockCount;
    bool runActive = false;

    auto flushRun = [&]()
    {
        if(!runActive || runBlocks == 0) return;

        markAllocatedRange(runStartBit, runBlocks);
        paging->MapArea(BitOffsetToAddr(runStartBit), runVirt, runBlocks, pageFlags);
        lastFreeBlockOffset = (runStartBit + runBlocks) % totalBlocks;
        currVirt = runVirt + runBlocks * PAGE_SIZE;
        runActive = false;
        runBlocks = 0;
    };

    auto appendSegment = [&](uint64_t segStartBit, size_t segLen) -> bool
    {
        if(segLen == 0 || blocksLeft == 0) return blocksLeft == 0;

        if(!runActive)
        {
            runActive = true;
            runStartBit = segStartBit;
            runVirt = currVirt;
            runBlocks = 0;
        }
        else if(segStartBit != runStartBit + runBlocks)
        {
            flushRun();
            runActive = true;
            runStartBit = segStartBit;
            runVirt = currVirt;
            runBlocks = 0;
        }

        size_t take = segLen < blocksLeft ? segLen : blocksLeft;
        runBlocks += take;
        blocksLeft -= take;

        if(blocksLeft == 0)
        {
            flushRun();
            return true;
        }

        return false;
    };

    auto scanPass = [&](uint64_t qStart, uint64_t qEnd, uint64_t bitShift) -> bool
    {
        for(uint64_t q = qStart; q < qEnd && blocksLeft > 0; q++)
        {
            uint64_t qword = bm[q];
            size_t windowBits = 64;

            if(q == qStart && bitShift != 0)
            {
                qword >>= bitShift;
                windowBits = 64 - bitShift;
            }

            uint64_t windowMask = windowBits == 64 ? UINT64_MAX : ((1ULL << windowBits) - 1ULL);
            qword &= windowMask;

            size_t localPos = 0;

            while(localPos < windowBits && blocksLeft > 0)
            {
                if(qword == 0)
                {
                    size_t segLen = windowBits - localPos;
                    if(appendSegment(q * 64 + bitShift + localPos, segLen)) return true;
                    break;
                }

                if(qword == windowMask) break;

                if(qword & 1ULL)
                {
                    uint64_t usedLen = __builtin_ctzll(~qword);

                    if(usedLen > windowBits - localPos) usedLen = windowBits - localPos;

                    qword >>= usedLen;
                    localPos += usedLen;

                    continue;
                }

                uint64_t freeLen = __builtin_ctzll(qword);
                if(freeLen > windowBits - localPos) freeLen = windowBits - localPos;

                if(appendSegment(q * 64 + bitShift + localPos, freeLen)) return true;

                qword >>= freeLen;
                localPos += freeLen;
            }
        }

        return false;
    };

    if(scanPass(searchStartQword, totalQwords, searchStartBit)) return MMD_SUCCESS;
    if(searchStartBlock != 0 && scanPass(0, searchStartQword, 0)) return MMD_SUCCESS;

    return MMD_OUT_OF_MEMORY;
}

bool PhysAlloc::FreeBlocks(void* base, size_t blockCount)
{
    if(blockCount == 0) return true;

    uint64_t bitOffset = AddrToBitOffset(reinterpret_cast<uintptr_t>(base));
    if(bitOffset == UINT64_MAX) return false;

    uint64_t* bitmap64 = reinterpret_cast<uint64_t*>(bitmap);

    uint64_t startQword = bitOffset >> 6;
    uint64_t startBit = bitOffset & 63;

    uint64_t endBit = startBit + blockCount;

    uint64_t fullQwords = endBit >> 6;
    uint64_t endRemainderBits = endBit & 63;
    
    if(startBit != 0)
    {
        uint64_t mask = (~0ULL << startBit);

        if(blockCount < (64 - startBit))
        {
            mask &= (~0ULL >> (64 - endBit));
            bitmap64[startQword] &= ~mask;
            return true;
        }

        bitmap64[startQword] &= ~mask;
        startQword++;
    }

    uint64_t endQword = fullQwords;

    for(uint64_t i = startQword; i < endQword; i++) bitmap64[i] = 0;

    if(endRemainderBits != 0)
    {
        uint64_t mask = (~0ULL >> (64 - endRemainderBits));
        bitmap64[endQword] &= ~mask;
    }

    if(bitOffset < lastFreeBlockOffset) lastFreeBlockOffset = bitOffset;

    return true;
}   

uintptr_t PhysAlloc::BitOffsetToAddr(uint64_t bitOffset)
{
    for(MemoryRange& range : memoryRanges)
    {
        if(range.length == 0) continue;

        uint64_t rangePageCount = BLOCK_COUNT(range.length);
        uint64_t rangeStart = range.bitmapIndexBase;
        uint64_t rangeEnd = rangeStart + rangePageCount;

        if(bitOffset >= rangeStart && bitOffset < rangeEnd)
        {
            uint64_t offsetPages = bitOffset - rangeStart;
            return range.start + offsetPages * PAGE_SIZE;
        }
    }

    return 0;
}

uint64_t PhysAlloc::AddrToBitOffset(uintptr_t addr)
{
    for(MemoryRange& range : memoryRanges)
    {
        if(range.length == 0) continue;

        uintptr_t rangeStartPhys = range.start;
        uintptr_t rangeEndPhys = range.start + range.length;

        if(addr >= rangeStartPhys && addr < rangeEndPhys)
        {
            uint64_t offsetBytes = addr - rangeStartPhys;
            uint64_t offsetPages = BLOCK_COUNT(offsetBytes);
            return range.bitmapIndexBase + offsetPages;
        }
    }

    return UINT64_MAX;
}