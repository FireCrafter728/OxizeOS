#pragma once

#include <stdint.hpp>

// Kernel reserved memory region allocator, region size is 64MiB, block size is 4K, a free list allocator that reserves memory regions for kernel-level data. Region already pre-mapped.

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 0x1000
#endif

#ifndef KIBIBYTE
#define KIBIBYTE 1024
#endif

#ifndef MEBIBYTE
#define MEBIBYTE KIBIBYTE * KIBIBYTE
#endif

namespace TskSchl
{
    namespace MMD
    {
        struct PACK KRNL_ListEntry
        {
            KRNL_ListEntry* next;
        };

        class KRNL
        {
        public:
            KRNL() = default;
            KRNL(uintptr_t RegionStart);
            bool Initialize(uintptr_t RegionStart);
            void* Allocate(size_t blocks, flags_t flags = PTE_PRESENT | PTE_RW);
            void Free(void* ptr, size_t blocks);
        private:
            KRNL_ListEntry* freeListStart;
            uintptr_t RegionStart;
            static constexpr size_t RegionSize = 64 * MEBIBYTE;
        };
    }
}