#include <disk.hpp>

using namespace FAT32;

bool DISK::Initialize(LPCSTR DiskImage)
{
    disk = fopen(DiskImage, "rb+");
    if(!disk) {
        fprintf(stderr, "[DISK] [ERROR]: Failed to open file %s\n", DiskImage);
        return false;
    }
    return true;
}

bool DISK::ReadSectors(m_uint32_t lba, m_uint16_t count, void* dataOut)
{
    fseek(disk, lba * SECTOR_SIZE, SEEK_SET);
    if(fread(dataOut, SECTOR_SIZE, count, disk) != count) {
        fprintf(stderr, "[DISK] [ERROR]: Failed to read from disk at lba %lu\n", lba);
        return false;
    }
    return true;
}

bool DISK::WriteSectors(m_uint32_t lba, m_uint16_t count, void* buffer)
{
    fseek(disk, lba * SECTOR_SIZE, SEEK_SET);
    if(fwrite(buffer, SECTOR_SIZE, count, disk) != count) {
        fprintf(stderr, "[DISK] [ERROR]: Failed to write data to disk at lba %lu\n", lba);
        return false;
    }
    return true;
}

DISK::~DISK()
{
    fclose(disk);
}