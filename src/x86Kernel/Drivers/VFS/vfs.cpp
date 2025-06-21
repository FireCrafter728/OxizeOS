#include <VFS/vfs.hpp>
#include <stdio.h>
#include <print.h>

extern "C" int VFS_Write(int file, uint8_t *data, size_t size)
{
    return x86Kernel::VFS::VFS::Write(file, data, size);
}

using namespace x86Kernel::VFS;

static VFS* instance;

VFS::VFS(FAT32::FAT32* fat, FAT32::File *fd)
{
    Initialize(fat, fd);
}

void VFS::Initialize(FAT32::FAT32* fat, FAT32::File *fd)
{
    instance = this;
    this->fd = fd;
    this->fat = fat;
}

int VFS::Write(fd_t file, uint8_t *data, size_t size)
{
    for (size_t i = 0; i < size; i++) print_putc(data[i]);
    return size;
}