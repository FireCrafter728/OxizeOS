#pragma once
#include <stdint.h>
#include <DISK/DISK.hpp>
#include <stdbool.h>

namespace x86Kernel
{
    namespace MBR
    {
        bool ReadSectors(DISK::DISK* disk, uint32_t lba, uint8_t sectors, void* dataOut);
    }
}