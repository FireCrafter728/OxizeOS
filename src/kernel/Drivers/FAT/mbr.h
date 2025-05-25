#pragma once
#include <stdint.h>
#include <DISK/DISK.h>
#include <stdbool.h>

bool MBR_ReadSectors(DISK* disk, uint32_t lba, uint8_t sectors, void* dataOut);