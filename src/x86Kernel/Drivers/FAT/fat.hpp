#pragma once
#include <stdint.h>
#include <stddef.h>
#include <FAT/mbr.hpp>

#define SECTOR_SIZE 512

namespace x86Kernel
{
    namespace FAT32
    {
        struct __attribute__((packed)) DirectoryEntry
        {
            uint8_t Name[11];
            uint8_t Attributes;
            uint8_t _Reserved;
            uint8_t CreatedTimeTenths;
            uint16_t CreatedTime;
            uint16_t CreatedDate;
            uint16_t AccessedDate;
            uint16_t FirstClusterHigh;
            uint16_t ModifiedTime;
            uint16_t ModifiedDate;
            uint16_t FirstClusterLow;
            uint32_t Size;
        };

        struct File
        {
            int Handle;
            bool IsDirectory;
            uint32_t Position;
            uint32_t Size;
        };

        enum Attribs{
            FAT_ATTRIBUTE_READ_ONLY = 0x01,
            FAT_ATTRIBUTE_HIDDEN = 0x02,
            FAT_ATTRIBUTE_SYSTEM = 0x04,
            FAT_ATTRIBUTE_VOLUME_ID = 0x08,
            FAT_ATTRIBUTE_DIRECTORY = 0x10,
            FAT_ATTRIBUTE_ARCHIVE = 0x20,
            FAT_ATTRIBUTE_LFN = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN | FAT_ATTRIBUTE_SYSTEM | FAT_ATTRIBUTE_VOLUME_ID
        };

        class FAT32
        {
        public:
            FAT32() = default;
            FAT32(MBR::MBR* mbr);
            bool Initialize(MBR::MBR* mbr);
            File* Open(const char* path);
            uint32_t Read(File* file, uint32_t byteCount, void* dataOut);
            void Seek(File* file, uint32_t Position);
            void ResetPos(File* file);
            bool ReadEntry(File* file, DirectoryEntry* entry);
            void Close(File* file);
            File* Create(const char* path);
            bool CreateDirectoryEntry(File* parent, const char* name);
            bool CreateEntry(File* directory, const char* name);
            ~FAT32() = default;
        private:
            uint32_t DataSectionLba;
            uint32_t TotalSectors;
            uint32_t FatStartLba;
            MBR::MBR* mbr;
            bool ReadBootSector();
            bool ReadFat(size_t lbaIndex);
            uint32_t ClusterToLba(uint32_t cluster);
            File* OpenEntry(DirectoryEntry* entry);
            uint32_t NextCluster(uint32_t currentCluster);
            void GetShortName(const char* fileName, char shortName[12]);
            bool FindFile(File* file, const char* name, DirectoryEntry* entryOut);
            uint32_t AllocateCluster();
            bool WriteDirectoryEntry(File* directory, const DirectoryEntry* newEntry);
            uint32_t ReadFatEntry(uint32_t cluster);
            bool WriteFatEntry(uint32_t cluster, uint32_t value);
        };
    }
}