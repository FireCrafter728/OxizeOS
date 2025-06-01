#pragma once
#include <DISK/SATA_AHCI.h>

typedef struct
{
    SATA_AHCI_Device device;
} DISK;

bool DISK_Initialize(DISK* disk);
bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint16_t count, void* dataOut);