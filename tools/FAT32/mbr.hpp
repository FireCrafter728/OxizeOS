#pragma once
#include <disk.hpp>

namespace FAT32
{
    namespace MBR
    {
        struct __attribute__((packed)) PartitionEntry
        {
            m_uint8_t driveAttribs;
            m_uint8_t partitionStartCHS[3];
            m_uint8_t partitionType;
            m_uint8_t partitionEndCHS[3];
            m_uint32_t partitionStartLBA;
            m_uint32_t partitionSectorCount;
        };

        struct __attribute__((packed)) BootSector
        {
            m_uint8_t Bootstrap[0x1BE];
            PartitionEntry partitionEntries[4];
            m_uint16_t bootSig;
        };

        struct MBRDesc
        {
            BootSector mbr = {};
            m_uint32_t bootPartitionStart = 0;
        };

        class MBR
        {
        public:
            bool Initialize(DISK *disk, MBRDesc *desc, int Partition = -1);
            bool ReadSectors(m_uint32_t lba, m_uint16_t count, void *dataOut);
            bool WriteSectors(m_uint32_t lba, m_uint16_t count, void *buffer);
            inline int GetPartitionIndex() {return CurrentPartition;};
            m_uint32_t GetPartitionStartLba(int partitionIndex = -1);
            m_uint32_t GetPartitionSectorCount(int partitionIndex = -1);
        private:
            MBRDesc *desc = nullptr;
            DISK *disk;
            int CurrentPartition;
        };
    }
}