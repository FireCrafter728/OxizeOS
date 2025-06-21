#pragma once
#include <stdint.h>
#include <stddef.h>
#include <FAT/fat.hpp>

#define VFS_FD_STDIN                0x01
#define VFS_FD_STDOUT               0x02
#define VFS_FD_STDERR               0x03
#define VFS_FD_DEBUG                0x04

namespace x86Kernel
{
    namespace VFS
    {
        typedef int fd_t;
        class VFS
        {
        public:
            VFS() = default;
            VFS(FAT32::FAT32* fat, FAT32::File* fd);
            void Initialize(FAT32::FAT32* fat, FAT32::File* fd);
            static int Write(fd_t file, uint8_t* data, size_t size);
        private:
            FAT32::File* fd;
            FAT32::FAT32* fat;
        };
    }
}