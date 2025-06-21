#pragma once
#include <stdio.h>

namespace FAT32
{
    class DISK
    {
    public:
        bool Initialize(LPCSTR DiskImage);
        bool ReadSectors(m_uint32_t lba, m_uint16_t count, void* dataOut);
        bool WriteSectors(m_uint32_t lba, m_uint16_t count, void* buffer);
        ~DISK();
    private:
        FILE* disk;
    };
}