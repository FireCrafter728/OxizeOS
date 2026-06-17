#include <Drivers/MMD/heap.hpp>

using namespace SysKrnl64::MMD;

MemoryAllocErrors HeapAlloc::Initialize(HeapAllocDesc* desc)
{
    if(!desc || !desc->virtAlloc || !desc->physAlloc) return MMD_INVALID_PARAMETER;

    static_assert(sizeof(AllocationHeader) % HEAP_ALIGNMENT == 0, "Drivers/MMD/heap.hpp: AllocationHeader must be 16-byte aligned");
    static_assert(INITIAL_HEAP_SIZE % BLOCK_SIZE == 0, "Drivers/MMD/heap.hpp: INITIAL_HEAP_SIZE must be block-aligned(4096)");

    this->desc = *desc;

    // Reserve a 1TiB virtual memory region for the heap

    auto virtReserveRes = desc->virtAlloc->AllocateBlocks(HEAP_RESERVE_BLOCKS, VA_NODE_FLAG_PHYSICALLY_NOT_BACKED | VA_NODE_FLAG_USED);
    if(!virtReserveRes)
    {
        printf("[SYSKRNL64] [HEAP ALLOC] [ERROR]: Failed to reserve initial kernel heap address space, error code: %d\r\n", virtReserveRes.error());
        return virtReserveRes.error();
    }
    heapVirtBase = reinterpret_cast<uintptr_t>(virtReserveRes.value());

    // Allocate an initial 64KiB of memory for the heap

    heapBlocks = BLOCK_COUNT(INITIAL_HEAP_SIZE);

    MemoryAllocErrors physAllocRes = desc->physAlloc->AllocSparseBlocksToContiguousVirtualRange(heapBlocks, heapVirtBase, PTE_PRESENT | PTE_RW);

    if(physAllocRes != MMD_SUCCESS)
    {
        printf("[SYSKRNL64] [HEAP ALLOC] [ERROR]: Failed to allocate 64KiB for initial kernel heap\r\n");
        return physAllocRes;
    }

    // Add a header to the heap to describe the free space

    AllocationHeader* allocHdr = reinterpret_cast<AllocationHeader*>(heapVirtBase);
    allocHdr->flags = 0;
    allocHdr->allocSize = INITIAL_HEAP_SIZE - sizeof(AllocationHeader);
    allocHdr->prev = nullptr;
    allocHdr->next = nullptr;

    lastHeader = allocHdr;

    return MMD_SUCCESS;
}

std::expected<void*, MemoryAllocErrors> HeapAlloc::AllocateBytes(size_t size)
{
    if(size == 0) return nullptr;

    size = ALIGN_UP(size, HEAP_ALIGNMENT);

    AllocationHeader* hdr = reinterpret_cast<AllocationHeader*>(heapVirtBase);
    const size_t heapBytes = heapBlocks * BLOCK_SIZE;

    while(hdr)
    {
        if(reinterpret_cast<uintptr_t>(hdr) + hdr->allocSize >= heapVirtBase + heapBytes) 
        {
            printf("[SYSKRNL64] [HEAP ALLOC] [ERROR]: Corrupted heap descriptor(s) data\r\n");    
            return std::unexpected<MemoryAllocErrors>(MMD_INVALID_STRUCTURE_DATA);
        }
        if(!(hdr->flags & HEAP_FLAG_USED))
        {
            // Make sure that the size is exact to the requested size or the left free space would be enough to fit a header and extra 16 free bytes
            if(hdr->allocSize == size || hdr->allocSize >= size + MINIMAL_FREE_HEAP_SIZE) break;
        }
        hdr = hdr->next;
    }

    if(!hdr)
    {
        // Expand the heap, then try again
        MemoryAllocErrors expandRes = ExpandHeap();
        if(expandRes != MMD_SUCCESS) {
            printf("[SYSKRNL64] [HEAP ALLOC] [ERROR]: Failed to expand the heap\r\n");
            return std::unexpected<MemoryAllocErrors>(expandRes);
        }
        return AllocateBytes(size);
    }

    AllocationHeader origHdr = *hdr;
    hdr->flags |= HEAP_FLAG_USED;
    hdr->allocSize = size;

    // No need to insert an extra header for free space if all free space was used up
    if(origHdr.allocSize - size == 0) return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(hdr) + sizeof(AllocationHeader));

    // Free space left afterwards
    AllocationHeader* nextFreeHdr = reinterpret_cast<AllocationHeader*>(reinterpret_cast<uintptr_t>(hdr) + sizeof(AllocationHeader) + hdr->allocSize);
    nextFreeHdr->flags = 0;
    nextFreeHdr->allocSize = origHdr.allocSize - hdr->allocSize - sizeof(AllocationHeader);
    nextFreeHdr->prev = hdr;
    nextFreeHdr->next = hdr->next;
    hdr->next = nextFreeHdr;
    if(nextFreeHdr->next) nextFreeHdr->next->prev = nextFreeHdr;

    if(hdr == lastHeader && hdr->next) lastHeader = nextFreeHdr;

    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(hdr) + sizeof(AllocationHeader));
}

MemoryAllocErrors HeapAlloc::FreeBytes(void* ptr)
{
    if(!ptr) return MMD_INVALID_PARAMETER;

    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    AllocationHeader* header = reinterpret_cast<AllocationHeader*>(addr - sizeof(AllocationHeader));
    
    if(addr < heapVirtBase + sizeof(AllocationHeader) || addr > heapVirtBase + heapBlocks * BLOCK_SIZE) return MMD_INVALID_PARAMETER;
    if(!(header->flags & HEAP_FLAG_USED)) return MMD_INVALID_PARAMETER;

    AllocationHeader* prev = header->prev;
    AllocationHeader* next = header->next;

    header->flags &= ~(HEAP_FLAG_USED);

    // Check if next allocation header marks free space

    if(next && !(next->flags & HEAP_FLAG_USED))
    {
        // Combine current and next allocation headers
        header->allocSize += next->allocSize + sizeof(AllocationHeader);
        header->next = next->next;
        if(next->next) next->next->prev = header;
        if(header->next == nullptr) lastHeader = header;
        return MMD_SUCCESS;
    }

    // Check if previous allocation header marks free space

    if(prev && !(prev->flags & HEAP_FLAG_USED))
    {
        // Combine the current and previous allocation headers
        prev->allocSize += header->allocSize + sizeof(AllocationHeader);
        prev->next = header->next;
        if(header->next) header->next->prev = prev;
        if(prev->next == nullptr) lastHeader = prev;
        return MMD_SUCCESS;
    }

    // Nothing can be combined

    return MMD_SUCCESS;
}

MemoryAllocErrors HeapAlloc::ExpandHeap()
{
    // Expand the heap
    size_t newHeapSizeBlocks = heapBlocks * 2;

    if(newHeapSizeBlocks > HEAP_RESERVE_BLOCKS) return MMD_OUT_OF_MEMORY;

    MemoryAllocErrors physAllocRes = desc.physAlloc->AllocSparseBlocksToContiguousVirtualRange(heapBlocks, heapVirtBase + heapBlocks * BLOCK_SIZE, PTE_PRESENT | PTE_RW);
    if(physAllocRes != MMD_SUCCESS)
    {
        printf("[SYSKRNL64] [HEAP ALLOC] [ERROR]: Failed to allocate extra blocks for the heap\r\n");
        return physAllocRes;
    }

    // Add an extra header, or modify the last header if it marks free space, to mark the new free space

    if(!(lastHeader->flags & HEAP_FLAG_USED))
    {
        // Modify the last header
        lastHeader->allocSize += heapBlocks * BLOCK_SIZE;
        heapBlocks = newHeapSizeBlocks;
        return MMD_SUCCESS;
    }

    AllocationHeader* hdr = reinterpret_cast<AllocationHeader*>(heapVirtBase + heapBlocks * BLOCK_SIZE);
    hdr->flags = 0;
    hdr->allocSize = heapBlocks * BLOCK_SIZE - sizeof(AllocationHeader);
    hdr->next = nullptr;
    hdr->prev = lastHeader;
    lastHeader->next = hdr;
    lastHeader = hdr;
    heapBlocks = newHeapSizeBlocks;
    return MMD_SUCCESS;
}

// Expose functions to stdio.hpp kmalloc, kcalloc & kfree

void* KernelAlloc(size_t size)
{
    auto res = SysKrnl64::heapAlloc->AllocateBytes(size);
    if(!res) return nullptr;
    return res.value();
}

void KernelFree(void* ptr)
{
    if(!ptr) return;
    SysKrnl64::heapAlloc->FreeBytes(ptr);
}