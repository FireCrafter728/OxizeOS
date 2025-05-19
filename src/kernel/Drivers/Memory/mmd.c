#include <Memory/mmd.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <stddef.h>

#define BIT32_MEMORY_START 1048576
#define GLOBAL_MEMORY_START 0x200000
#define SECTOR_SIZE 512

void MMD_Initialize(MMD_MemoryDriver *driver, MemoryInfo memoryInfo)
{
    driver->memoryInfo = memoryInfo;
    // memset(driver->ELFFiles, 0, sizeof(driver->ELFFiles));
    driver->MemoryRegions = memoryInfo.RegionCount;
}

typedef struct
{
    uint32_t LocationSize;
    uint32_t LocationStart;
    uint8_t LocationType;
} MMD_MemoryLocation;

int CompareMemoryLocationsBySize(const void *a, const void *b)
{
    const MMD_MemoryLocation *locA = (const MMD_MemoryLocation *)a;
    const MMD_MemoryLocation *locB = (const MMD_MemoryLocation *)b;

    if (locA->LocationSize < locB->LocationSize)
        return -1;
    if (locA->LocationSize > locB->LocationSize)
        return 1;
    return 0;
}

uint32_t MMD_FindMemoryLocation(MMD_MemoryDriver *driver, uint32_t Size)
{
    MMD_MemoryLocation memoryLocations[driver->MemoryRegions];
    int validRegions = 0;

    for (int i = 0; i < driver->MemoryRegions; i++)
    {
        MemoryRegion *region = &driver->memoryInfo.Regions[i];
        if (region->Type != 1) continue; // Usable memory only

        uint64_t regionStart = region->Begin;
        uint64_t regionEnd = region->Begin + region->Length;

        if (regionEnd <= GLOBAL_MEMORY_START) continue;

        if (regionStart < GLOBAL_MEMORY_START)
            regionStart = GLOBAL_MEMORY_START;

        uint64_t adjustedLength = regionEnd - regionStart;
        if (adjustedLength < Size) continue;

        memoryLocations[validRegions].LocationStart = (uint32_t)regionStart;
        memoryLocations[validRegions].LocationSize = (uint32_t)adjustedLength;
        memoryLocations[validRegions].LocationType = region->Type;
        validRegions++;
    }

    // for (int i = 0; i < driver->MemoryRegions; i++)
    // {
    //     MemoryRegion *region = &driver->memoryInfo.Regions[i];
    //     if (region->Length < Size)
    //         continue;
    //     if (region->Type != 1)
    //         continue; // Only allow usable memory
    //     if (region->Begin < BIT32_MEMORY_START)
    //         continue; // Only allow memory past the 20bit address limit

    //     memoryLocations[validRegions].LocationStart = region->Begin + GLOBAL_MEMORY_START;
    //     memoryLocations[validRegions].LocationSize = region->Length - GLOBAL_MEMORY_START;
    //     memoryLocations[validRegions].LocationType = region->Type;
    //     validRegions++;
    // }

    qsort(memoryLocations, validRegions, sizeof(MMD_MemoryLocation), CompareMemoryLocationsBySize);

    printf("[MMD] [INFO]: Input params: Size: 0x%x", Size);

    printf("[MMD] [INFO]: memoryLocation 0 data: Start: 0x%x, Size: 0x%x, Type: %d\r\n", memoryLocations[0].LocationStart, memoryLocations[0].LocationSize, memoryLocations[0].LocationType);

    if (validRegions > 0)
    {
        return memoryLocations[0].LocationStart;
    }

    return 0;
}

// int MMD_AddElfFile(MMD_MemoryDriver *driver, ELF_File *elf)
// {
//     int index = -1;
//     for (int i = 0; i < MAX_EXECUTABLE_COUNT && index < 0; i++)
//         if (driver->ELFFiles[i] = NULL)
//             index = i;
//     if (index < 0)
//         return index;
//     driver->ELFFiles[index] = elf;
//     return index;
// }

// void MMD_RemoveElfFile(MMD_MemoryDriver *driver, uint8_t index)
// {
//     memset(driver->ELFFiles[index], 0, sizeof(driver->ELFFiles[index]));
// }