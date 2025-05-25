#pragma once
#include <DISK/ATA_PIO.h>
#include <DISK/SATA_AHCI.h>

typedef enum
{
    DISK_CONTROLLER_IDE,
    DISK_CONTROLLER_SATA_IDE,
    DISK_CONTROLLER_SATA_AHCI,
} DISK_DeviceTypes;

typedef struct
{
    ATA_PIO_Device ATAPio;
    SATA_AHCI_Device SATAAhci;
    uint8_t DeviceType;
} DISK;

bool DISK_Initialize(DISK* disk);
bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint8_t count, void* dataOut);