#include <FAT/mbr.h>
#include <memory.h>
#include <stdio.h>

#define PARTITION_START 0x800

bool MBR_ReadSectors(ATA_PIO_Device* device, uint32_t lba, uint8_t sectors, void* lowerDataOut)
{
    return ATA_PIO_ReadSectors(device, lba + PARTITION_START, sectors, lowerDataOut);
}