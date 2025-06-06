#include <FAT/mbr.hpp>
#include <stdio.h>

using namespace x86Kernel;

#define PARTITION_START 0x800

bool MBR::ReadSectors(DISK::DISK* disk, uint32_t lba, uint8_t sectors, void* dataOut)
{
    return disk->ReadSectors(lba + PARTITION_START, sectors, dataOut);
}