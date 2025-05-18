#pragma once
#include <stdint.h>
#include <DISK/ATA_PIO.h>
#include <stdbool.h>

bool MBR_ReadSectors(ATA_PIO_Device* device, uint32_t lba, uint8_t sectors, void* lowerDataOut);