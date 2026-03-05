#pragma once

#include <stdint.hpp>

// Memory Mapped I/O Allocator(MMIO), a virtual memory bump allocator that reserves virtual memory regions for MMIO

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 0x1000
#endif

namespace TskSchl
{
    namespace MMD
    {
        class MMIO
        {
        public:
            MMIO() = default;
            MMIO(uintptr_t VirtStart, size_t length);
            bool Initialize(uintptr_t VirtStart, size_t length);
            void* Allocate(size_t Blocks);
            void Free(size_t Blocks);
        private:
            uint8_t* ptr;
            uint8_t* end;
        };
    }
}