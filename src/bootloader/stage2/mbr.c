#include "mbr.h"
#include "memory.h"
#include "stdio.h"

void MBR_DetectPartition(MBR_Partition* Partition, DISK* disk, void* partition)
{
    Partition->disk = disk;
    if(Partition->disk->id < 0x80) {
        Partition->partitionOffset = 0;
        Partition->partitionSize = (uint32_t)(disk->cylinders) * (uint32_t)(disk->heads) * (uint32_t)(disk->sectors);
    } else {
        MBR_PartitionEntry* mainPartition = (MBR_PartitionEntry*) SegOffToLinear(partition);
        Partition->partitionOffset = mainPartition->PartitionStartLBA;
        Partition->partitionSize = mainPartition->PartitionSectorCount;
        printf("[MBR] [INFO]: Partition detected: Attributes: 0x%x, Partition type: 0x%x, Start LBA: 0x%x, Sector count: 0x%x\r\n", mainPartition->DriveAttribs, mainPartition->PartitionType, mainPartition->PartitionStartLBA, mainPartition->PartitionSectorCount);
    }
}

bool MBR_ReadSectors(MBR_Partition* Partition, uint32_t lba, uint8_t sectors, void* lowerDataOut)
{
    return DISK_ReadSectors(Partition->disk, lba + Partition->partitionOffset, sectors, lowerDataOut);
}