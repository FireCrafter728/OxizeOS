#pragma once

#include <stdint.h>
#include <stdbool.h>

#define SECTOR_SIZE_WORD                        256

typedef struct {
    uint8_t model[41];
    uint32_t sectorCount;
} ATA_PIO_DriveInfo;

typedef struct {
    uint16_t IdentifyData[SECTOR_SIZE_WORD];
    ATA_PIO_DriveInfo driveInfo;
    bool Initialized;
} ATA_PIO_Device;

bool ATA_PIO_Initialize(ATA_PIO_Device* device);

bool ATA_PIO_ReadSectors(ATA_PIO_Device* device, uint32_t lba, uint8_t count, void* dataOut);