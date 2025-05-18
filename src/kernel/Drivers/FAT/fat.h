#pragma once
#include <stdint.h>
#include <FAT/mbr.h>

#define SECTOR_SIZE             512

typedef struct 
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
} __attribute__((packed)) FAT_DirectoryEntry;

typedef struct 
{
    int Handle;
    bool IsDirectory;
    uint32_t Position;
    uint32_t Size;
} FAT_File;

typedef enum 
{
    FAT_ATTRIBUTE_READ_ONLY         = 0x01,
    FAT_ATTRIBUTE_HIDDEN            = 0x02,
    FAT_ATTRIBUTE_SYSTEM            = 0x04,
    FAT_ATTRIBUTE_VOLUME_ID         = 0x08,
    FAT_ATTRIBUTE_DIRECTORY         = 0x10,
    FAT_ATTRIBUTE_ARCHIVE           = 0x20,
    FAT_ATTRIBUTE_LFN               = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN | FAT_ATTRIBUTE_SYSTEM | FAT_ATTRIBUTE_VOLUME_ID
} FAT_Attributes;

bool FAT_Initialize(ATA_PIO_Device* device);
FAT_File * FAT_Open(ATA_PIO_Device* device, const char* path);
uint32_t FAT_Read(ATA_PIO_Device* device, FAT_File* file, uint32_t byteCount, void* dataOut);
void FAT_Seek(FAT_File* file, uint32_t Position);
void FAT_ResetPos(FAT_File* file);
bool FAT_ReadEntry(ATA_PIO_Device* device, FAT_File* file, FAT_DirectoryEntry* dirEntry);
void FAT_Close(FAT_File* file);
