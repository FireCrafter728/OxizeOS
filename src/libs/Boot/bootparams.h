#pragma once
#include <stdint.h>

typedef struct {
    uint64_t Begin, Length;
    uint32_t Type;
    uint32_t ACPI;
} MemoryRegion;

typedef struct {
    int RegionCount;
    MemoryRegion* Regions;
} MemoryInfo;

typedef struct
{
    uint16_t bootDrive;
    MemoryInfo memoryInfo;
    uint32_t ISRHandlers;
} SystemInfo;
