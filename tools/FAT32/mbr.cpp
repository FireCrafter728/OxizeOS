#include <mbr.hpp>

using namespace FAT32::MBR;

bool MBR::Initialize(DISK *disk, MBRDesc *desc, int Partition)
{
    this->desc = desc;
    this->disk = disk;
    // m_uint8_t buffer[SECTOR_SIZE];
    // if (!disk->ReadSectors(0, 1, &buffer))
    // {
    //     printf("[MBR] [ERROR]: Failed to read MBR\n");
    //     return false;
    // }

    if (!disk->ReadSectors(0, 1, &desc->mbr))
    {
        printf("[MBR] [ERROR]: Failed to read MBR\n");
        return false;
    }

    if (Partition < 0)
    {
        for (int i = 0; i < MBR_MAX_PARTITIONS; i++)
            if (desc->mbr.partitionEntries[i].driveAttribs == MBR_PARTITION_BOOTABLE)
            {
                desc->bootPartitionStart = desc->mbr.partitionEntries[i].partitionStartLBA;
                this->CurrentPartition = i;
                break;
            }

        if (desc->bootPartitionStart == 0)
        {
            printf("[MBR] [ERROR]: Failed to find bootable partition\n");
            return false;
        }
    }
    else if(Partition > 4) {
        fprintf(stderr, "[MBR] [ERROR]: Unsupported partition %d\n", Partition);
        return false;
    }
    else {
        desc->bootPartitionStart = desc->mbr.partitionEntries[Partition - 1].partitionStartLBA;
        this->CurrentPartition = Partition;
    }

    return true;
}

bool MBR::ReadSectors(m_uint32_t lba, m_uint16_t count, void *dataOut)
{
    return disk->ReadSectors(lba + desc->bootPartitionStart, count, dataOut);
}

bool MBR::WriteSectors(m_uint32_t lba, m_uint16_t count, void *buffer)
{
    return disk->WriteSectors(lba + desc->bootPartitionStart, count, buffer);
}

m_uint32_t MBR::GetPartitionStartLba(int partitionIndex)
{
    if(partitionIndex == -1) return desc->mbr.partitionEntries[CurrentPartition - 1].partitionStartLBA;
    return desc->mbr.partitionEntries[partitionIndex - 1    ].partitionStartLBA;
}

m_uint32_t MBR::GetPartitionSectorCount(int partitionIndex)
{
    if(partitionIndex == -1) return desc->mbr.partitionEntries[CurrentPartition - 1].partitionSectorCount;
    return desc->mbr.partitionEntries[partitionIndex - 1].partitionSectorCount;
}