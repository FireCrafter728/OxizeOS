#include <FAT/mbr.hpp>
#include <stdio.h>
#include <io.h>

using namespace x86Kernel::MBR;

#define MBR_PARTITIONS                      4

MBR::MBR(DISK::DISK* disk, MBRDesc* mbr)
{
    if(!Initialize(disk, mbr)) HaltSystem();
}

bool MBR::Initialize(DISK::DISK* disk, MBRDesc* mbr)
{
    this->desc = mbr;
    this->disk = disk;
    if(!disk->ReadSectors(0, 1, &mbr->mbr)) {
        printf("[MBR] [ERROR]: Failed to read MBR in disk\r\n");
        return false;
    }

    for(int i = 0; MBR_PARTITIONS; i++) {
        if(mbr->mbr.partitionEntries[i].driveAttribs == 0x80) {
            printf("[MBR] [INFO]: Found bootable partition %d, partition start LBA: %lu, partition sector count: %lu\r\n", i, mbr->mbr.partitionEntries[i].partitionStartLBA, mbr->mbr.partitionEntries[i].partitionSectorCount);
            mbr->bootPartitionStart = mbr->mbr.partitionEntries[i].partitionStartLBA;
            break;
        }
    }

    if(mbr->bootPartitionStart == 0) {
        printf("[MBR] [ERROR]: No bootable MBR partition found\r\n");
        return false;
    }

    return true;
}

bool MBR::ReadSectors(uint32_t lba, uint16_t sectors, void *dataOut)
{
    return disk->ReadSectors(lba + desc->bootPartitionStart, sectors, dataOut);
}

bool MBR::WriteSectors(uint32_t lba, uint16_t sectors, const void *buffer)
{
    return disk->WriteSectors(lba + desc->bootPartitionStart, sectors, buffer);
}