#pragma once
#include <stdint.h>
#include <DISK/DISK.hpp>

namespace x86Kernel
{
    namespace MBR
    {
        struct __attribute__((packed)) PartitionEntry {
            uint8_t driveAttribs;
            uint8_t partitionStartCHS[3];
            uint8_t partitionType;
            uint8_t partitionEndCHS[3];
            uint32_t partitionStartLBA;
            uint32_t partitionSectorCount;
        };

        struct __attribute__((packed)) BootSector {
            uint8_t Bootstrap[0x1BE];
            PartitionEntry partitionEntries[4];
            uint16_t bootSig;
        };

        struct MBRDesc {
            BootSector mbr = {};
            uint32_t bootPartitionStart = 0;
        };

        class MBR
        {
        public:
            MBR() = default;
            MBR(DISK::DISK* disk, MBRDesc* mbr);
            bool Initialize(DISK::DISK* disk, MBRDesc* mbr);
            bool ReadSectors(uint32_t lba, uint16_t sectors, void* dataOut);
            bool WriteSectors(uint32_t lba, uint16_t sectors, const void* buffer);
        private:
            DISK::DISK* disk;
            MBRDesc* desc;
        };
    }
}