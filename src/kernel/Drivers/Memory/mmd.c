#include <Memory/mmd.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <stddef.h>
#include <Memory/memdefs.h>

#define SECTOR_SIZE 512

void MMD_Initialize(MMD_MemoryDriver *driver, MemoryInfo memoryInfo)
{
    driver->memoryInfo = memoryInfo;
    memset(driver->ELFFiles, 0, sizeof(driver->ELFFiles));
    driver->MemoryRegions = memoryInfo.RegionCount;
    driver->ELFFileCount = 0;
}

typedef struct
{
    uint64_t LocationSize;
    uint64_t LocationStart;
    uint8_t LocationType;
} MMD_MemoryLocation;

typedef struct
{
    uint64_t RegionSize;
    uint64_t RegionStart;
} MMD_ELFRegion;

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

uint64_t MMD_FindMemoryLocation(MMD_MemoryDriver *driver, uint64_t Size)
{
    MMD_MemoryLocation memoryLocations[driver->MemoryRegions];
    int validRegions = 0;

    for (int i = 0; i < driver->MemoryRegions; i++)
    {
        MemoryRegion *region = &driver->memoryInfo.Regions[i];
        if (region->Type != 1)
            continue; // Usable memory only

        uint64_t regionStart = region->Begin;
        uint64_t regionEnd = region->Begin + region->Length;

        if (regionEnd <= GLOBAL_MEMORY_START)
            continue;

        if (regionStart < GLOBAL_MEMORY_START)
            regionStart = GLOBAL_MEMORY_START;

        uint64_t adjustedLength = regionEnd - regionStart;
        if (adjustedLength < Size)
            continue;

        memoryLocations[validRegions].LocationStart = regionStart;
        memoryLocations[validRegions].LocationSize = adjustedLength;
        memoryLocations[validRegions].LocationType = region->Type;
        validRegions++;
    }

    qsort(memoryLocations, validRegions, sizeof(MMD_MemoryLocation), CompareMemoryLocationsBySize);

    MMD_ELFRegion ELFRegions[MAX_EXECUTABLE_COUNT];
    uint16_t pos = 0;

    for (int i = 0; i < driver->ELFFileCount; i++)
    {
        ELF_File *elf = driver->ELFFiles[i];
        if (!elf)
            continue; // Current position doesn't contain a executable entry
        ELFRegions[pos].RegionStart = elf->LoadAddr;
        ELFRegions[pos].RegionSize = elf->LoadSize;
        pos++;
    }

    if (validRegions > 0)
    {
        for (uint16_t i = 0; i < validRegions; i++)
        {
            uint16_t OverlappingRegions[MAX_EXECUTABLE_COUNT];
            uint16_t pos2 = 0;

            // Collect overlapping ELF regions
            for (uint16_t j = 0; j < driver->ELFFileCount; j++)
            {
                if ((memoryLocations[i].LocationStart < ELFRegions[j].RegionStart + ELFRegions[j].RegionSize) &&
                    (ELFRegions[j].RegionStart < memoryLocations[i].LocationStart + memoryLocations[i].LocationSize))
                {
                    OverlappingRegions[pos2++] = j;
                }
            }

            // If no overlapping ELF regions, whole region is free
            if (pos2 == 0)
            {
                if (memoryLocations[i].LocationSize >= Size)
                    return memoryLocations[i].LocationStart;
                else
                    continue; // Not enough size, check next memory region
            }

            // Sort overlapping ELF regions by start address (simple bubble sort here)
            for (int a = 0; a < pos2 - 1; a++)
            {
                for (int b = a + 1; b < pos2; b++)
                {
                    if (ELFRegions[OverlappingRegions[b]].RegionStart < ELFRegions[OverlappingRegions[a]].RegionStart)
                    {
                        uint16_t temp = OverlappingRegions[a];
                        OverlappingRegions[a] = OverlappingRegions[b];
                        OverlappingRegions[b] = temp;
                    }
                }
            }

            // Check free space before the first overlapping ELF region
            uint64_t freeStart = memoryLocations[i].LocationStart;
            uint64_t freeEnd = ELFRegions[OverlappingRegions[0]].RegionStart;
            if (freeEnd > freeStart && (freeEnd - freeStart) >= Size)
                return freeStart;

            // Check free space between overlapping ELF regions
            for (int k = 0; k < pos2 - 1; k++)
            {
                freeStart = ELFRegions[OverlappingRegions[k]].RegionStart + ELFRegions[OverlappingRegions[k]].RegionSize;
                freeEnd = ELFRegions[OverlappingRegions[k + 1]].RegionStart;

                if (freeEnd > freeStart && (freeEnd - freeStart) >= Size)
                    return freeStart;
            }

            // Check free space after the last overlapping ELF region within the memory region
            freeStart = ELFRegions[OverlappingRegions[pos2 - 1]].RegionStart + ELFRegions[OverlappingRegions[pos2 - 1]].RegionSize;
            freeEnd = memoryLocations[i].LocationStart + memoryLocations[i].LocationSize;

            if (freeEnd > freeStart && (freeEnd - freeStart) >= Size)
                return freeStart;

            // No suitable free space in this region, continue to next
        }
    }

    return 0;
}

int MMD_AddElfFile(MMD_MemoryDriver* driver, ELF_File* elf)
{
    int index = -1;
    for (int i = 0; i < MAX_EXECUTABLE_COUNT && index < 0; i++)
        if (driver->ELFFiles[i] == NULL)
            index = i;
    if (index < 0)
        return index;
    driver->ELFFiles[index] = elf;
    driver->ELFFileCount++;
    return index;
}

void MMD_RemoveElfFile(MMD_MemoryDriver *driver, int index)
{
    memset(driver->ELFFiles[index], 0, sizeof(driver->ELFFiles[index]));
    driver->ELFFileCount--;
}