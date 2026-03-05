#include <Drivers/MMD/krnl.hpp>

using namespace TskSchl::MMD;

KRNL::KRNL(uintptr_t RegionStart)
{
    if(!Initialize(RegionStart)) HaltSystem();
}

bool KRNL::Initialize(uintptr_t RegionStart)
{
    if(!RegionStart) {
        printf("[TSKSCHL] [MMD-KRNL] [ERROR]: Invalid Initializer Args\r\n");
        return false;
    }

    this->RegionStart = RegionStart;

    // Setup free list

    size_t totalBlocks = RegionSize / BLOCK_SIZE;
    KRNL_ListEntry* Start = nullptr;

    for(size_t i = 0; i < totalBlocks; i++)
    {
        KRNL_ListEntry* block = reinterpret_cast<KRNL_ListEntry*>(RegionStart + i * BLOCK_SIZE);
        block->next = Start;
        Start = block;
    }

    this->freeListStart = Start;

    return true;
}

void* KRNL::Allocate(size_t blocks)
{
    if(!blocks) {
        printf("[TSKSCHL] [MMD-KRNL] [ERROR]: An attempt was made to allocate 0 blocks\r\n");
        return nullptr;
    }

    KRNL_ListEntry* prev = nullptr, *current = this->freeListStart, *runStart = nullptr;

    size_t runLength = 0;

    while(current)
    {
        if(runLength == 0 || reinterpret_cast<uintptr_t>(current) == reinterpret_cast<uintptr_t>(runStart) + runLength * BLOCK_SIZE) {
            if(runLength == 0) runStart = current;
            runLength++;

            if(runLength == blocks)
            {
                KRNL_ListEntry* runEndNext = current->next;

                if(prev) prev->next = runEndNext;
                else freeListStart = runEndNext;

                KRNL_ListEntry* temp = runStart;
                for(size_t i = 0; i < blocks; i++) {
                    temp->next = nullptr;
                    temp = reinterpret_cast<KRNL_ListEntry*>(reinterpret_cast<uintptr_t>(temp) + BLOCK_SIZE);
                }

                memset(reinterpret_cast<void*>(runStart), 0, blocks * BLOCK_SIZE);

                return runStart;
            }
        }
        else {
            runStart = current;
            runLength = 1;
        }

        prev = current;
        current = current->next;
    }

    printf("[TSKSCHL] [MMD-KRNL] [ERROR]: Failed to allocate memory: Out of memory!\r\n");
    return nullptr;
}

void KRNL::Free(void* ptr, size_t blocks)
{
    if(!ptr || !blocks) {
        printf("[TSKSCHL] [MMD-KRNL] [ERROR]: An attempt was made to free a region of data at address NULL or 0 blocks\r\n");
        return;
    }

    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    
    for(size_t i = 0; i < blocks; i++)
    {
        KRNL_ListEntry* block = reinterpret_cast<KRNL_ListEntry*>(addr + i * BLOCK_SIZE);
        block->next = freeListStart;
        freeListStart = block;
    }
}