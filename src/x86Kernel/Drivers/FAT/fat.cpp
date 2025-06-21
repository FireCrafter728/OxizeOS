#include <FAT/fat.hpp>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <algorithm>
#include <Memory/memdefs.hpp>
#include <io.h>

using namespace x86Kernel::FAT32;

#define MAX_PATH_SIZE 256
#define MAX_FILE_HANDLES 64
#define ROOT_DIRECTORY_HANDLE -1
#define CACHE_SIZE 5

struct __attribute__((packed)) BootSector
{
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

static Data *data;

bool FAT32::ReadBootSector()
{
    return this->mbr->ReadSectors(0, 1, data->BS.BootSectorBytes);
}

bool FAT32::ReadFat(size_t lbaIndex)
{
    return this->mbr->ReadSectors(data->BS.bootSector.ReservedSectors + lbaIndex, CACHE_SIZE, data->FatCache);
}

uint32_t FAT32::ClusterToLba(uint32_t cluster)
{
    return DataSectionLba + (cluster - 2) * data->BS.bootSector.SectorsPerCluster;
}

FAT32::FAT32(MBR::MBR *mbr)
{
    if (!Initialize(mbr))
        HaltSystem();
}

bool FAT32::Initialize(MBR::MBR *mbr)
{
    this->mbr = mbr;
    data = (Data *)MEMORY_FAT_ADDR;

    if (!FAT32::ReadBootSector())
    {
        printf("[FAT] [ERROR]: Failed to read boot sector\r\n");
        return false;
    }

    data->FatCachePos = 0xFFFFFFFF;

    TotalSectors = (data->BS.bootSector.TotalSectors == 0) ? data->BS.bootSector.LargeSectorCount : data->BS.bootSector.TotalSectors;

    this->DataSectionLba = data->BS.bootSector.ReservedSectors + data->BS.bootSector.FAT32_SectorsPerFat * data->BS.bootSector.FatCount;
    this->FatStartLba = data->BS.bootSector.ReservedSectors;
    uint32_t rootDirLba = FAT32::ClusterToLba(data->BS.bootSector.FAT32_RootdirCluster);
    uint32_t rootDirSize = 0;

    data->RootDirectory.Public.Handle = ROOT_DIRECTORY_HANDLE;
    data->RootDirectory.Public.IsDirectory = true;
    data->RootDirectory.Public.Position = 0;
    data->RootDirectory.Public.Size = 0;
    data->RootDirectory.Opened = true;
    data->RootDirectory.FirstCluster = rootDirLba;
    data->RootDirectory.CurrentCluster = rootDirLba;
    data->RootDirectory.CurrentSectorInCluster = 0;

    if (!this->mbr->ReadSectors(rootDirLba, 1, data->RootDirectory.Buffer))
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

File *FAT32::OpenEntry(DirectoryEntry *entry)
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

    FileData *fd = &data->OpenedFiles[handle];
    fd->Public.Handle = handle;
    fd->Public.IsDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->Public.Position = 0;
    fd->Public.Size = entry->Size;
    fd->FirstCluster = entry->FirstClusterLow + ((uint32_t)entry->FirstClusterHigh << 16);
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;

    if (!this->mbr->ReadSectors(FAT32::ClusterToLba(fd->CurrentCluster), 1, fd->Buffer))
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
    if (fatIndexSector < data->FatCachePos || fatIndexSector >= data->FatCachePos + CACHE_SIZE)
    {
        FAT32::ReadFat(fatIndexSector);
        data->FatCachePos = fatIndexSector;
    }

    fatIndex -= (data->FatCachePos * SECTOR_SIZE);

    return *(uint32_t *)(data->FatCache + fatIndex);
}

uint32_t FAT32::Read(File *file, uint32_t byteCount, void *dataOut)
{
    FileData *fd = (file->Handle == ROOT_DIRECTORY_HANDLE)
                       ? &data->RootDirectory
                       : &data->OpenedFiles[file->Handle];

    uint8_t *u8DataOut = (uint8_t *)dataOut;

    if (!fd->Public.IsDirectory)
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
            uint32_t nextSectorOffset;

            if (fd->Public.Handle == ROOT_DIRECTORY_HANDLE)
            {
                ++fd->CurrentCluster;
                nextSectorOffset = fd->CurrentCluster * SECTOR_SIZE;
            }
            else
            {
                if (++fd->CurrentSectorInCluster >= data->BS.bootSector.SectorsPerCluster)
                {
                    fd->CurrentSectorInCluster = 0;
                    fd->CurrentCluster = NextCluster(fd->CurrentCluster);
                }

                if (fd->CurrentCluster >= 0x0FFFFFFF)
                {
                    fd->Public.Size = fd->Public.Position;
                    break;
                }

                nextSectorOffset = (ClusterToLba(fd->CurrentCluster) - ClusterToLba(data->BS.bootSector.FAT32_RootdirCluster)) * SECTOR_SIZE + fd->CurrentSectorInCluster * SECTOR_SIZE;
            }

            if (nextSectorOffset >= fd->Public.Size)
                break;
            uint32_t nextLba;
            if (fd->Public.Handle == ROOT_DIRECTORY_HANDLE)
                nextLba = fd->CurrentCluster;
            else
                nextLba = ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster;
            if (!this->mbr->ReadSectors(nextLba, 1, fd->Buffer))
            {
                printf("FAT: Read error\r\n");
                break;
            }
        }
    }

    return u8DataOut - (uint8_t *)dataOut;
}

bool FAT32::ReadEntry(File *file, DirectoryEntry *dirEntry)
{
    return Read(file, sizeof(DirectoryEntry), dirEntry) == sizeof(DirectoryEntry);
}

void FAT32::Seek(File *file, uint32_t Position)
{
    FileData *fd = &data->OpenedFiles[file->Handle];
    file->Position = Position;
    uint32_t clusterIndex = Position / (data->BS.bootSector.SectorsPerCluster * data->BS.bootSector.BytesPerSector);
    uint32_t cluster = fd->FirstCluster;
    for (uint32_t i = 0; i < clusterIndex; i++)
    {
        cluster = NextCluster(cluster);
        if (cluster >= 0x0FFFFFF8)
        {
            printf("[FAT SEEK] [ERROR]: Cluster is too high\r\n");
            return;
        }
    }

    fd->CurrentCluster = cluster;
    uint32_t clusterOff = Position % (data->BS.bootSector.SectorsPerCluster * data->BS.bootSector.BytesPerSector);
    uint32_t sector = clusterOff / data->BS.bootSector.BytesPerSector;
    uint32_t lba = ClusterToLba(cluster) + sector;
    if (!mbr->ReadSectors(lba, 1, fd->Buffer))
    {
        printf("[FAT SEEK] [ERROR]: Failed to read from disk\n\n");
        return;
    }
    fd->CurrentSectorInCluster = lba % data->BS.bootSector.SectorsPerCluster;
}

void FAT32::ResetPos(File *file)
{
    FileData *fd = &data->OpenedFiles[file->Handle];
    file->Position = 0;
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;
    if (!mbr->ReadSectors(ClusterToLba(fd->FirstCluster), 1, fd->Buffer))
        printf("[FAT SEEK] [ERROR]: Failed to read from disk\n\n");
}

void FAT32::Close(File *file)
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

void FAT32::GetShortName(const char *fileName, char shortName[11])
{
    memset(shortName, ' ', 11);

    const char *ext = strchr(fileName, '.');
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

bool FAT32::FindFile(File *file, const char *name, DirectoryEntry *entryOut)
{
    char shortName[11];
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

File *FAT32::Open(const char *path)
{
    char name[MAX_PATH_SIZE];

    if (path[0] == '/')
        path++;

    File *current = &data->RootDirectory.Public;

    while (*path)
    {
        bool isLast = false;
        const char *delim = strchr(path, '/');
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

File *FAT32::Create(const char *path)
{
    File *file = nullptr;
    char fatName[MAX_PATH_SIZE];

    if (path[0] == '/')
        path++;

    File *current = &data->RootDirectory.Public;

    int index = 0;

    while (*path)
    {
        bool isLast = false;
        const char *delim = strchr(path, '/');
        size_t len;

        if (delim != nullptr)
            len = delim - path;
        else
        {
            len = strlen(path);
            isLast = true;
        }

        memcpy(fatName, path, len);
        fatName[len] = '\0';

        DirectoryEntry entry;
        bool found = FindFile(current, fatName, &entry);
        printf("found: %u\r\n", found);

        if (found)
        {
            if (isLast)
            {
                printf("FAT: Create: File %s already exists, returning existing file handle\r\n", path);
                return OpenEntry(&entry);
            }
            Close(current);
            if (!isLast && !(entry.Attributes & FAT_ATTRIBUTE_DIRECTORY))
            {
                printf("FAT: %s not a directory\r\n", fatName);
                return nullptr;
            }

            current = OpenEntry(&entry);
        }
        else
        {
            if (!isLast)
            {
                if (!CreateDirectoryEntry(current, fatName))
                {
                    Close(current);
                    printf("FAT: Failed to create a directory %s\r\n", fatName);
                    return nullptr;
                }
                if (!FindFile(current, fatName, &entry))
                {
                    Close(current);
                    printf("FAT: Failed to find file %s\r\n", fatName);
                    return nullptr;
                }
                Close(current);
                current = OpenEntry(&entry);
            }
            else
            {
                if (!CreateEntry(current, fatName))
                {
                    Close(current);
                    printf("FAT: Failed to create entry %s\r\n", fatName);
                    return nullptr;
                }

                if (!FindFile(current, fatName, &entry))
                {
                    Close(current);
                    printf("FAT: Failed to find file %s\r\n", fatName);
                    return nullptr;
                }

                file = OpenEntry(&entry);
                Close(current);
                return file;
            }
        }

        path += len;
        if (*path == '/')
            path++;
    }

    return file;
}

bool FAT32::CreateDirectoryEntry(File *parent, const char *name)
{
    uint32_t newCluster = AllocateCluster();
    if (newCluster == 0)
    {
        printf("FAT: Failed to allocate new clusters\r\n");
        return false;
    }
    DirectoryEntry dirEntry = {};
    char shortName[12];
    GetShortName(name, shortName);
    memcpy(dirEntry.Name, shortName, 11);
    dirEntry.Attributes = FAT_ATTRIBUTE_DIRECTORY;
    dirEntry.FirstClusterLow = newCluster & 0xFFFF;
    dirEntry.FirstClusterHigh = (newCluster >> 16) & 0xFFFF;
    dirEntry.Size = 0;

    if (!WriteDirectoryEntry(parent, &dirEntry))
    {
        printf("FAT: Failed to write a new directory %s\r\n", name);
        return false;
    }

    DirectoryEntry dot = {};
    memcpy(dot.Name, ".          ", 11);
    dot.Attributes = FAT_ATTRIBUTE_DIRECTORY;
    dot.FirstClusterLow = newCluster & 0xFFFF;
    dot.FirstClusterHigh = (newCluster >> 16) & 0xFFFF;
    dot.Size = 0;

    DirectoryEntry dotdot = {};
    memcpy(dotdot.Name, "..         ", 11);
    dotdot.Attributes = FAT_ATTRIBUTE_DIRECTORY;

    if (parent->IsDirectory)
    {
        uint32_t parentCluster = data->OpenedFiles[parent->Handle].FirstCluster;
        dotdot.FirstClusterLow = parentCluster & 0xFFFF;
        dotdot.FirstClusterHigh = (parentCluster >> 16) & 0xFFFF;
    }
    else
    {
        dotdot.FirstClusterLow = 0;
        dotdot.FirstClusterHigh = 0;
    }

    uint8_t buffer[SECTOR_SIZE] = {};
    memcpy(buffer, &dot, sizeof(DirectoryEntry));
    memcpy(buffer + sizeof(DirectoryEntry), &dotdot, sizeof(DirectoryEntry));

    uint32_t dirLba = ClusterToLba(newCluster);
    if (!mbr->WriteSectors(dirLba, 1, buffer))
    {
        printf("FAT: Failed to write directory entries(current & parentDirs)\r\n");
        return false;
    }

    return true;
}

uint32_t FAT32::AllocateCluster()
{
    const uint32_t fatSectorCount = data->BS.bootSector.FAT32_SectorsPerFat;
    const uint32_t entriesPerSector = SECTOR_SIZE / 4;
    uint8_t sector[SECTOR_SIZE];

    for (uint32_t sectorIndex = 0; sectorIndex < fatSectorCount; sectorIndex++)
    {
        if (!mbr->ReadSectors(FatStartLba + sectorIndex, 1, sector))
        {
            printf("FAT: Failed to read FAT sector %u\r\n", sectorIndex);
            return 0;
        }

        uint32_t *entries = (uint32_t *)sector;

        for (uint32_t i = 0; i < entriesPerSector; i++)
        {
            uint32_t cluster = sectorIndex * entriesPerSector + i;
            if (cluster < 2)
                continue;

            if ((entries[i] & 0x0FFFFFFF) == 0)
            {
                entries[i] = 0x0FFFFFFF;

                for (uint32_t fatIndex = 0; fatIndex < data->BS.bootSector.FatCount; fatIndex++)
                {
                    uint32_t fatLba = FatStartLba + fatIndex * fatSectorCount + sectorIndex;
                    if (!mbr->WriteSectors(fatLba, 1, sector))
                    {
                        printf("FAT: Failed to write fat sector %u\r\n", sectorIndex);
                        return 0;
                    }
                }

                uint8_t zeroBuf[SECTOR_SIZE];
                memset(zeroBuf, 0, SECTOR_SIZE);
                uint32_t clusterLba = ClusterToLba(cluster);
                for (uint32_t s = 0; s < data->BS.bootSector.SectorsPerCluster; s++)
                {
                    if (!mbr->WriteSectors(clusterLba + s, 1, zeroBuf))
                    {
                        printf("FAT: Failed to zero out cluster %u\r\n", cluster);
                        return 0;
                    }
                }

                return cluster;
            }
        }
    }

    printf("FAT: no free clusters avaiable\r\n");
    return 0;
}

bool FAT32::CreateEntry(File *directory, const char *name)
{
    DirectoryEntry entry = {};
    char shortName[12];

    GetShortName(name, shortName);
    memcpy(entry.Name, shortName, 11);

    entry.Attributes = 0;
    entry.Size = 0;

    uint32_t cluster = AllocateCluster();
    if (cluster == 0)
    {
        printf("FAT: Failed to allocate cluster for file %s\r\n", name);
        return false;
    }

    entry.FirstClusterLow = cluster & 0xFFFF;
    entry.FirstClusterHigh = (cluster >> 16) & 0xFFFF;

    if (!WriteDirectoryEntry(directory, &entry))
    {
        printf("FAT: Failed to write directory entry for file %s\r\n", name);
        return false;
    }

    return true;
}

bool FAT32::WriteDirectoryEntry(File *directory, const DirectoryEntry *newEntry)
{
    uint8_t sectorBuffer[SECTOR_SIZE];
    FileData *fd = &data->OpenedFiles[directory->Handle];

    uint32_t cluster = fd->FirstCluster;

    if (cluster == 0)
        cluster = 2;
    while (true)
    {
        for (uint32_t sectorIndex = 0; sectorIndex < data->BS.bootSector.SectorsPerCluster; sectorIndex++)
        {
            uint32_t sectorLba = ClusterToLba(cluster) + sectorIndex;

            if (!mbr->ReadSectors(sectorLba, 1, sectorBuffer))
            {
                printf("FAT: Failed to read directory sector %u\r\n", sectorLba);
                return false;
            }

            DirectoryEntry *entries = (DirectoryEntry *)sectorBuffer;

            for (uint32_t i = 0; i < SECTOR_SIZE / sizeof(DirectoryEntry); i++)
            {
                if (entries[i].Name[0] == 0x00 || entries[i].Name[0] == 0xE5)
                {
                    memcpy(&entries[i], newEntry, sizeof(DirectoryEntry));

                    if (!mbr->WriteSectors(sectorLba, 1, sectorBuffer))
                    {
                        printf("FAT: Failed to write directory sector %u\r\n", sectorLba);
                        return false;
                    }
                    return true;
                }
            }
        }

        uint32_t nextCluster = ReadFatEntry(cluster);

        if (nextCluster == 0x0FFFFFFF)
        {
            uint32_t newCluster = AllocateCluster();
            if (newCluster == 0)
            {
                printf("FAT: Failed to allocate cluster for directory expansion\r\n");
                return false;
            }

            if (!WriteFatEntry(cluster, newCluster))
            {
                printf("FAT: Failed to chain new cluster in FAT\r\n");
                return false;
            }

            if (!WriteFatEntry(newCluster, 0x0FFFFFFF))
            {
                printf("FAT: Failed to mark new cluster as EOC\r\n");
                return false;
            }

            uint8_t zeroBuf[SECTOR_SIZE];
            memset(zeroBuf, 0, SECTOR_SIZE);
            uint32_t clusterLba = ClusterToLba(newCluster);

            for (uint32_t s = 0; s < data->BS.bootSector.SectorsPerCluster; s++)
            {
                if (!mbr->WriteSectors(clusterLba + s, 1, zeroBuf))
                {
                    printf("FAT: Failed to zero out new cluster %u\r\n", newCluster);
                    return false;
                }
            }

            memcpy((DirectoryEntry *)zeroBuf, newEntry, sizeof(DirectoryEntry));

            if (!mbr->WriteSectors(clusterLba, 1, zeroBuf))
            {
                printf("FAT: Failed to write directory entry in new cluster\r\n");
                return false;
            }

            return true;
        }
        else
            cluster = nextCluster;
    }
}

uint32_t FAT32::ReadFatEntry(uint32_t cluster)
{
    uint32_t fatIndex = cluster * 4;
    uint32_t fatIndexSector = fatIndex / SECTOR_SIZE;

    if (fatIndexSector < data->FatCachePos || fatIndexSector >= data->FatCachePos + CACHE_SIZE)
    {
        if (!ReadFat(fatIndexSector))
        {
            printf("FAT: Failed to read FAT sectors at %u\r\n", fatIndexSector);
            HaltSystem();
        }
        data->FatCachePos = fatIndexSector;
    }

    uint32_t offsetInCache = fatIndex - (data->FatCachePos * SECTOR_SIZE);
    uint32_t entry;
    memcpy(&entry, data->FatCache + offsetInCache, sizeof(uint32_t));
    return entry & 0x0FFFFFFF;
}

bool FAT32::WriteFatEntry(uint32_t cluster, uint32_t value)
{
    uint32_t fatIndex = cluster * 4;
    uint32_t fatIndexSector = fatIndex / SECTOR_SIZE;

    if (fatIndexSector < data->FatCachePos || fatIndexSector >= data->FatCachePos + CACHE_SIZE)
    {
        if (!ReadFat(fatIndexSector))
        {
            printf("FAT: Failed to read FAT sectors at %u\r\n", fatIndexSector);
            return false;
        }
        data->FatCachePos = fatIndexSector;
    }

    uint32_t offsetInCache = fatIndex - (data->FatCachePos * SECTOR_SIZE);

    uint32_t origEntry;
    memcpy(&origEntry, data->FatCache + offsetInCache, sizeof(uint32_t));
    uint32_t updatedEntry = (origEntry & 0xF0000000) | (value & 0x0FFFFFFF);
    memcpy(data->FatCache + offsetInCache, &updatedEntry, sizeof(uint32_t));

    const uint32_t fatSectorCount = data->BS.bootSector.FAT32_SectorsPerFat;
    for (uint32_t fatCopy = 0; fatCopy < data->BS.bootSector.FatCount; fatCopy++)
    {
        uint32_t baseLba = FatStartLba + fatCopy * fatSectorCount + data->FatCachePos;

        if (!mbr->WriteSectors(baseLba, CACHE_SIZE, data->FatCache))
        {
            printf("FAT: Failed to write FAT sectors at %u for fat copy %u\r\n", data->FatCachePos, fatCopy);
            return false;
        }
    }

    return true;
}