#include "memdetect.h"
#include "x86.h"
#include "stdio.h"

MemoryRegion memoryRegions[MAX_MEMORY_REGIONS];
int memoryRegionCount;

void Memory_Detect(MemoryInfo* info)
{
    E820MemoryBlock block;
    uint32_t continuation = 0;
    int ret;

    memoryRegionCount = 0;
    ret = x86_E820GetNextBlock(&block, &continuation);

    while(ret > 0 && continuation != 0) {
        memoryRegions[memoryRegionCount].Begin = block.Base;
        memoryRegions[memoryRegionCount].Length = block.Length;
        memoryRegions[memoryRegionCount].Type = block.Type;
        memoryRegions[memoryRegionCount].ACPI = block.ACPI;
        memoryRegionCount++;

        printf("[MEMDETECT] [INFO]: Memory region found: Base: 0x%llx, Length: 0x%llx, Type: 0x%x\r\n", block.Base, block.Length, block.Type);

        ret = x86_E820GetNextBlock(&block, &continuation);
    }

    info->RegionCount = memoryRegionCount;
    info->Regions = memoryRegions;
}