#include <FAT/mbr.h>
#include <memory.h>
#include <stdio.h>

#define PARTITION_START 0x800

bool MBR_ReadSectors(DISK* disk, uint32_t lba, uint8_t sectors, void* dataOut)
{
    // printf("\r\nlba: %lu, partition_lba: %lu, sectors: %u", lba + PARTITION_START, lba, sectors);
    return DISK_ReadSectors(disk, lba + PARTITION_START, sectors, dataOut);
}