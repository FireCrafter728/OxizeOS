// SPDX-License-Identifier: GPL-3.0-or-later

#include <disk.hpp>

using namespace FAT32;

FAT32_STATUS DISK::Initialize(const char* DiskImage)
{
    disk = fopen(DiskImage, "rb+");
    if(!disk) {
        fprintf(stderr, "[FAT32] [DISK] [ERROR]: Failed to open file %s\n", DiskImage);
        return FAT32_DISK_ERROR;
    }
    return FAT32_SUCCESS;
}

FAT32_STATUS DISK::ReadSectors(uint64_t lba, size_t count, void* dataOut)
{
    fseeko(disk, lba * SECTOR_SIZE, SEEK_SET);
    if(fread(dataOut, SECTOR_SIZE, count, disk) != count) {
        fprintf(stderr, "[FAT32] [DISK] [ERROR]: Failed to read from disk at lba %lu\n", lba);
        return FAT32_DISK_READ_ERROR;
    }
    return FAT32_SUCCESS;
}

FAT32_STATUS DISK::WriteSectors(uint64_t lba, size_t count, void* buffer)
{
    fseeko(disk, lba * SECTOR_SIZE, SEEK_SET);
    if(fwrite(buffer, SECTOR_SIZE, count, disk) != count) {
        fprintf(stderr, "[FAT32] [DISK] [ERROR]: Failed to write data to disk at lba %lu\n", lba);
        return FAT32_DISK_WRITE_ERROR;
    }
    return FAT32_SUCCESS;
}

uint64_t DISK::GetDiskSectorCount()
{
	if(fseeko(disk, 0, SEEK_END) != 0) return 0;
	off_t totalSize = ftello(disk);
	if(totalSize < 0) return 0;
	fseeko(disk, 0, SEEK_SET);
	return static_cast<uint64_t>(totalSize) / SECTOR_SIZE;
}

DISK::~DISK()
{
    fclose(disk);
}