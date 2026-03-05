#pragma once

#include <Drivers/MMD/mmio.hpp>
#include <Drivers/MMD/krnl.hpp>

namespace TskSchl
{
    namespace MMD
    {
        enum MemoryTypes
        {
            MT_MMIO,
            MT_KRNL,
        };

        class MMD
        {
        public:
            MMD() = default;
            MMD(MMIO* mmio, KRNL* krnl);
            bool Initialize(MMIO* mmio, KRNL* krnl);
            void* malloc(size_t blocks, MemoryTypes mt);
            void free(void* ptr, size_t blocks, MemoryTypes mt);
            void freeBlocks(size_t blocks, MemoryTypes mt);
        private:
            MMIO* mmio;
            KRNL* krnl;
        };
    }
}