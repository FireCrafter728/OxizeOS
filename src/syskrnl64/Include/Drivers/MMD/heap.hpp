#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |----------------------------------------------------------------------------| //
// | OxizeOS Kernel Implementation                                              | //
// | HEAP ALLOCATOR: A heap allocator that allocates memory for the kernel only | //
// |----------------------------------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdint.hpp>

#include <Drivers/MMD/memdefs.hpp>
#include <Drivers/MMD/virt.hpp>

namespace SysKrnl64
{
    namespace MMD
    {
        constexpr uint64_t HEAP_ALIGNMENT = 16;
        constexpr uint64_t INITIAL_HEAP_SIZE = 16 * BLOCK_SIZE; // 64KiB initial heap
        constexpr uint64_t HEAP_RESERVE_BLOCKS = 0x10000000; // Reserve 1TiB virtual address space for the heap. Not all of it is backed by physical memory, but can be expanded on demand

        enum AllocationHeaderFlags
        {
            // Bit 0: is region free or not? 0: free, 1: used
            HEAP_FLAG_USED = (1ULL << 0),
            
            
        };

        struct PACK AllocationHeader
        {
            size_t allocSize;
            AllocationHeader* prev;
            AllocationHeader* next;
            uint64_t flags;
        };

        constexpr uint64_t MINIMAL_FREE_HEAP_SIZE = sizeof(AllocationHeader) + HEAP_ALIGNMENT;

        struct HeapAllocDesc
        {
            VirtAlloc* virtAlloc;
            PhysAlloc* physAlloc;
        };

        class HeapAlloc
        {
        public:
            HeapAlloc() = default;
            MemoryAllocErrors Initialize(HeapAllocDesc* desc);
            std::expected<void*, MemoryAllocErrors> AllocateBytes(size_t size);
            MemoryAllocErrors FreeBytes(void* ptr);
        private:
            MemoryAllocErrors ExpandHeap();
            uintptr_t heapVirtBase;
            size_t heapBlocks;
            HeapAllocDesc desc;
            AllocationHeader* lastHeader;
        };
    }
}