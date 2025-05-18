#pragma once
#include <stdint.h>
#include "disk.h"
#include <stdbool.h>

typedef struct {
    uint8_t DriveAttribs;
    uint8_t PartitionStartCHS[3];
    uint8_t PartitionType;
    uint8_t LastPartitionSectorCHS[3];
    uint32_t PartitionStartLBA;
    uint32_t PartitionSectorCount;
} __attribute__((packed)) MBR_PartitionEntry;

typedef struct {
    DISK* disk;
    uint32_t partitionOffset;
    uint32_t partitionSize;
} MBR_Partition;

void MBR_DetectPartition(MBR_Partition* Partition, DISK* disk, void* partition);
bool MBR_ReadSectors(MBR_Partition* Partition, uint32_t lba, uint8_t sectors, void* lowerDataOut);