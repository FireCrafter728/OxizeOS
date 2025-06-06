#include <FAT/fat.hpp>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <algorithm>
#include <Memory/memdefs.hpp>
#include <io.h>

using namespace x86Kernel::FAT32;

#define MAX_PATH_SIZE           256
#define MAX_FILE_HANDLES        10
#define ROOT_DIRECTORY_HANDLE   -1
#define CACHE_SIZE              5

struct __attribute__((packed)) BootSector{
    uint8_t BootJumpInstruction[3];
    uint8_t OemIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t LargeSectorCount;

    uint32_t FAT32_SectorsPerFat;
    uint16_t FAT32_Flags;
    uint16_t FAT32_VersionNumber;
    uint32_t FAT32_RootdirCluster;
    uint16_t FAT32_FSInfoSector;
    uint16_t FAT32_BackupBootSector;
    uint8_t FAT32_Reserved[12];
    uint8_t FAT32_DriveNumber;
    uint8_t FAT32_NTFlags;
    uint8_t FAT32_Signature;
    uint8_t FAT32_DiskSerialNumber;
    uint8_t FAT32_VolumeLabel[11];
    uint8_t FAT32_SystemID[8];
};

struct FileData
{
    uint8_t Buffer[SECTOR_SIZE];
    File Public;
    bool Opened;
    uint32_t FirstCluster;
    uint32_t CurrentCluster;
    uint32_t CurrentSectorInCluster;
};

struct Data
{
    union
    {
        BootSector bootSector;
        uint8_t BootSectorBytes[SECTOR_SIZE];
    } BS;

    FileData RootDirectory;

    FileData OpenedFiles[MAX_FILE_HANDLES];

    uint8_t FatCache[CACHE_SIZE * SECTOR_SIZE];
    uint32_t FatCachePos;
};

static Data* data;

bool FAT32::ReadBootSector()
{
    return MBR::ReadSectors(this->disk, 0, 1, data->BS.BootSectorBytes);
}

bool FAT32::ReadFat(size_t lbaIndex)
{
    return MBR::ReadSectors(this->disk, data->BS.bootSector.ReservedSectors + lbaIndex, CACHE_SIZE, data->FatCache);
}

uint32_t FAT32::ClusterToLba(uint32_t cluster)
{
    return DataSectionLba + (cluster - 2) * data->BS.bootSector.SectorsPerCluster;
}

FAT32::FAT32(DISK::DISK* disk)
{
    if(!Initialize(disk)) HaltSystem();
}

bool FAT32::Initialize(DISK::DISK* disk)
{
    this->disk = disk;
    data = (Data*)MEMORY_FAT_ADDR;

    if (!FAT32::ReadBootSector())
    {
        printf("[FAT] [ERROR]: Failed to read boot sector\r\n");
        return false;
    }

    data->FatCachePos = 0xFFFFFFFF;

    TotalSectors = (data->BS.bootSector.TotalSectors == 0) ? data->BS.bootSector.LargeSectorCount : data->BS.bootSector.TotalSectors;

    this->DataSectionLba = data->BS.bootSector.ReservedSectors + data->BS.bootSector.FAT32_SectorsPerFat * data->BS.bootSector.FatCount;
    uint32_t rootDirLba = FAT32::ClusterToLba(data->BS.bootSector.FAT32_RootdirCluster);
    uint32_t rootDirSize = 0;

    data->RootDirectory.Public.Handle = ROOT_DIRECTORY_HANDLE;
    data->RootDirectory.Public.IsDirectory = true;
    data->RootDirectory.Public.Position = 0;
    data->RootDirectory.Public.Size = sizeof(DirectoryEntry) * data->BS.bootSector.DirEntryCount;
    data->RootDirectory.Opened = true;
    data->RootDirectory.FirstCluster = rootDirLba;
    data->RootDirectory.CurrentCluster = rootDirLba;
    data->RootDirectory.CurrentSectorInCluster = 0;

    if (!MBR::ReadSectors(this->disk, rootDirLba, 1, data->RootDirectory.Buffer))
    {
        printf("FAT: read root directory failed\r\n");
        return false;
    }

    uint32_t rootDirSectors = (rootDirSize + data->BS.bootSector.BytesPerSector - 1) / data->BS.bootSector.BytesPerSector;
    this->DataSectionLba = rootDirLba + rootDirSectors;

    for (int i = 0; i < MAX_FILE_HANDLES; i++)
        data->OpenedFiles[i].Opened = false;

    return true;
}

File* FAT32::OpenEntry(DirectoryEntry* entry)
{
    int handle = -1;
    for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++)
    {
        if (!data->OpenedFiles[i].Opened)
            handle = i;
    }

    if (handle < 0)
    {
        printf("FAT: out of file handles\r\n");
        return nullptr;
    }

    FileData* fd = &data->OpenedFiles[handle];
    fd->Public.Handle = handle;
    fd->Public.IsDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->Public.Position = 0;
    fd->Public.Size = entry->Size;
    fd->FirstCluster = entry->FirstClusterLow + ((uint32_t)entry->FirstClusterHigh << 16);
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;

    if (!MBR::ReadSectors(this->disk, FAT32::ClusterToLba(fd->CurrentCluster), 1, fd->Buffer))
    {
        printf("FAT: open entry failed - read error cluster=%u lba=%u\n", fd->CurrentCluster, FAT32::ClusterToLba(fd->CurrentCluster));
        for (int i = 0; i < 11; i++)
            printf("%c", entry->Name[i]);
        printf("\n");
        return nullptr;
    }

    fd->Opened = true;
    return &fd->Public;
}

uint32_t FAT32::NextCluster(uint32_t currentCluster)
{    
    uint32_t fatIndex = currentCluster * 4;

    uint32_t fatIndexSector = fatIndex / SECTOR_SIZE;
    if(fatIndexSector < data->FatCachePos || fatIndexSector >= data->FatCachePos + CACHE_SIZE) {
        FAT32::ReadFat(fatIndexSector);
        data->FatCachePos = fatIndexSector;
    }

    fatIndex -= (data->FatCachePos * SECTOR_SIZE);

    return *(uint32_t*)(data->FatCache + fatIndex);
}

uint32_t FAT32::Read(File* file, uint32_t byteCount, void* dataOut)
{
    FileData* fd = (file->Handle == ROOT_DIRECTORY_HANDLE) 
        ? &data->RootDirectory 
        : &data->OpenedFiles[file->Handle];

    uint8_t* u8DataOut = (uint8_t*)dataOut;

    if (!fd->Public.IsDirectory || (fd->Public.IsDirectory && fd->Public.Size != 0))
        byteCount = std::min(byteCount, fd->Public.Size - fd->Public.Position);

    while (byteCount > 0)
    {
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->Public.Position % SECTOR_SIZE);
        uint32_t take = std::min(byteCount, leftInBuffer);

        memcpy(u8DataOut, fd->Buffer + fd->Public.Position % SECTOR_SIZE, take);
        u8DataOut += take;
        fd->Public.Position += take;
        byteCount -= take;

        if (leftInBuffer == take)
        {
            if (fd->Public.Handle == ROOT_DIRECTORY_HANDLE)
            {
                ++fd->CurrentCluster;

                if (!MBR::ReadSectors(this->disk, fd->CurrentCluster, 1, fd->Buffer))
                {
                    printf("FAT: read error!\r\n");
                    break;
                }
            }
            else
            {
                if (++fd->CurrentSectorInCluster >= data->BS.bootSector.SectorsPerCluster)
                {
                    fd->CurrentSectorInCluster = 0;
                    fd->CurrentCluster = NextCluster(fd->CurrentCluster);
                }

                if (fd->CurrentCluster >= 0xFFFFFFF8)
                {
                    fd->Public.Size = fd->Public.Position;
                    break;
                }

                if (!MBR::ReadSectors(this->disk, ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster, 1, fd->Buffer))
                {
                    printf("FAT: read error!\r\n");
                    break;
                }
            }
        }
    }

    return u8DataOut - (uint8_t*)dataOut;
}

bool FAT32::ReadEntry(File* file, DirectoryEntry* dirEntry)
{
    return Read(file, sizeof(DirectoryEntry), dirEntry) == sizeof(DirectoryEntry);
}

void FAT32::Seek(File* file, uint32_t Position)
{
    file->Position = Position;
}

void FAT32::ResetPos(File* file)
{
    file->Position = 0;
}

void FAT32::Close(File* file)
{
    if (file->Handle == ROOT_DIRECTORY_HANDLE)
    {
        file->Position = 0;
        data->RootDirectory.CurrentCluster = data->RootDirectory.FirstCluster;
    }
    else
    {
        data->OpenedFiles[file->Handle].Opened = false;
    }
}

void FAT32::GetShortName(const char* fileName, char shortName[12])
{
    memset(shortName, ' ', 12);
    shortName[11] = '\0';

    const char* ext = strchr(fileName, '.');
    if (ext == NULL)
        ext = fileName + 11;

    for (int i = 0; i < 8 && fileName[i] && fileName + i < ext; i++)
    shortName[i] = toupper(fileName[i]);

    if (ext != fileName + 11)
    {
        for (int i = 0; i < 3 && ext[i + 1]; i++)
        shortName[i + 8] = toupper(ext[i + 1]);
    }
}

bool FAT32::FindFile(File* file, const char* name, DirectoryEntry* entryOut)
{
    char shortName[12];
    DirectoryEntry entry;

    GetShortName(name, shortName);

    while (ReadEntry(file, &entry))
    {
        if (memcmp(shortName, entry.Name, 11) == 0)
        {
            *entryOut = entry;
            return true;
        }        
    }
    
    return false;
}

File* FAT32::Open(const char* path)
{
    char name[MAX_PATH_SIZE];

    if (path[0] == '/')
        path++;

    File* current = &data->RootDirectory.Public;

    while (*path) {
        bool isLast = false;
        const char* delim = strchr(path, '/');
        if (delim != NULL)
        {
            memcpy(name, path, delim - path);
            name[delim - path] = '\0';
            path = delim + 1;
        }
        else
        {
            unsigned len = strlen(path);
            memcpy(name, path, len);
            name[len + 1] = '\0';
            path += len;
            isLast = true;
        }
        
        DirectoryEntry entry;
        if (FindFile(current, name, &entry))
        {
            Close(current);

            if (!isLast && (entry.Attributes & FAT_ATTRIBUTE_DIRECTORY) == 0)
            {
                printf("FAT: %s not a directory\r\n", name);
                return nullptr;
            }

            current = OpenEntry(&entry);
        }
        else
        {
            Close(current);

            printf("FAT: %s not found\r\n", name);
            return nullptr;
        }
    }

    return current;
}
