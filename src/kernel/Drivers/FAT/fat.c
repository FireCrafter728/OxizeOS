#include <FAT/fat.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <stddef.h>
#include <minmax.h>
#include <Memory/memdefs.h>

#define MAX_PATH_SIZE           256
#define MAX_FILE_HANDLES        10
#define ROOT_DIRECTORY_HANDLE   -1
#define FAT_CACHE_SIZE          5

typedef struct {
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
    uint16_t FAT32_FAT_VersionNumber;
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
} __attribute__((packed)) FAT_BootSector;

typedef struct
{
    uint8_t Buffer[SECTOR_SIZE];
    FAT_File Public;
    bool Opened;
    uint32_t FirstCluster;
    uint32_t CurrentCluster;
    uint32_t CurrentSectorInCluster;
} FAT_FileData;

typedef struct
{
    union
    {
        FAT_BootSector BootSector;
        uint8_t BootSectorBytes[SECTOR_SIZE];
    } BS;

    FAT_FileData RootDirectory;

    FAT_FileData OpenedFiles[MAX_FILE_HANDLES];

    uint8_t FatCache[FAT_CACHE_SIZE * SECTOR_SIZE];
    uint32_t FatCachePos;
} FAT_Data;

static FAT_Data* g_Data;
static uint32_t g_DataSectionLba;
static uint32_t TotalSectors;

bool FAT_ReadBootSector(DISK* disk)
{
    return MBR_ReadSectors(disk, 0, 1, g_Data->BS.BootSectorBytes);
}

bool FAT_ReadFat(DISK* disk, size_t lbaIndex)
{
    return MBR_ReadSectors(disk, g_Data->BS.BootSector.ReservedSectors + lbaIndex, FAT_CACHE_SIZE, g_Data->FatCache);
}

uint32_t FAT_ClusterToLba(uint32_t cluster)
{
    return g_DataSectionLba + (cluster - 2) * g_Data->BS.BootSector.SectorsPerCluster;
}

bool FAT_Initialize(DISK* disk)
{
    g_Data = (FAT_Data*)MEMORY_FAT_ADDR;

    if (!FAT_ReadBootSector(disk))
    {
        printf("FAT: read boot sector failed\r\n");
        return false;
    }

    g_Data->FatCachePos = 0xFFFFFFFF;

    TotalSectors = (g_Data->BS.BootSector.TotalSectors == 0) ? g_Data->BS.BootSector.LargeSectorCount : g_Data->BS.BootSector.TotalSectors;

    g_DataSectionLba = g_Data->BS.BootSector.ReservedSectors + g_Data->BS.BootSector.FAT32_SectorsPerFat * g_Data->BS.BootSector.FatCount;
    uint32_t rootDirLba = FAT_ClusterToLba(g_Data->BS.BootSector.FAT32_RootdirCluster);
    uint32_t rootDirSize = 0;

    g_Data->RootDirectory.Public.Handle = ROOT_DIRECTORY_HANDLE;
    g_Data->RootDirectory.Public.IsDirectory = true;
    g_Data->RootDirectory.Public.Position = 0;
    g_Data->RootDirectory.Public.Size = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;
    g_Data->RootDirectory.Opened = true;
    g_Data->RootDirectory.FirstCluster = rootDirLba;
    g_Data->RootDirectory.CurrentCluster = rootDirLba;
    g_Data->RootDirectory.CurrentSectorInCluster = 0;

    if (!MBR_ReadSectors(disk, rootDirLba, 1, g_Data->RootDirectory.Buffer))
    {
        printf("FAT: read root directory failed\r\n");
        return false;
    }

    uint32_t rootDirSectors = (rootDirSize + g_Data->BS.BootSector.BytesPerSector - 1) / g_Data->BS.BootSector.BytesPerSector;
    g_DataSectionLba = rootDirLba + rootDirSectors;

    for (int i = 0; i < MAX_FILE_HANDLES; i++)
        g_Data->OpenedFiles[i].Opened = false;

    return true;
}

FAT_File* FAT_OpenEntry(DISK* disk, FAT_DirectoryEntry* entry)
{
    int handle = -1;
    for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++)
    {
        if (!g_Data->OpenedFiles[i].Opened)
            handle = i;
    }

    if (handle < 0)
    {
        printf("FAT: out of file handles\r\n");
        return false;
    }

    FAT_FileData* fd = &g_Data->OpenedFiles[handle];
    fd->Public.Handle = handle;
    fd->Public.IsDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->Public.Position = 0;
    fd->Public.Size = entry->Size;
    fd->FirstCluster = entry->FirstClusterLow + ((uint32_t)entry->FirstClusterHigh << 16);
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;

    if (!MBR_ReadSectors(disk, FAT_ClusterToLba(fd->CurrentCluster), 1, fd->Buffer))
    {
        printf("FAT: open entry failed - read error cluster=%u lba=%u\n", fd->CurrentCluster, FAT_ClusterToLba(fd->CurrentCluster));
        for (int i = 0; i < 11; i++)
            printf("%c", entry->Name[i]);
        printf("\n");
        return false;
    }

    fd->Opened = true;
    return &fd->Public;
}

uint32_t FAT_NextCluster(DISK* disk, uint32_t currentCluster)
{    
    uint32_t fatIndex = currentCluster * 4;

    uint32_t fatIndexSector = fatIndex / SECTOR_SIZE;
    if(fatIndexSector < g_Data->FatCachePos || fatIndexSector >= g_Data->FatCachePos + FAT_CACHE_SIZE) {
        FAT_ReadFat(disk, fatIndexSector);
        g_Data->FatCachePos = fatIndexSector;
    }

    fatIndex -= (g_Data->FatCachePos * SECTOR_SIZE);

    return *(uint32_t*)(g_Data->FatCache + fatIndex);
}

uint32_t FAT_Read(DISK* disk, FAT_File* file, uint32_t byteCount, void* dataOut)
{
    // printf("NEW READ, pos: %lu, size: %lu || ", file->Position, file->Size);
    FAT_FileData* fd = (file->Handle == ROOT_DIRECTORY_HANDLE) 
        ? &g_Data->RootDirectory 
        : &g_Data->OpenedFiles[file->Handle];

    uint8_t* u8DataOut = (uint8_t*)dataOut;

    // printf("NEW READ, LBA: %d\r\n", (fd->Public.Position + byteCount + 511) / 512 + FAT_ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster + 0x800);

    if (!fd->Public.IsDirectory || (fd->Public.IsDirectory && fd->Public.Size != 0))
        byteCount = min(byteCount, fd->Public.Size - fd->Public.Position);

    while (byteCount > 0)
    {
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->Public.Position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);
        memcpy(u8DataOut, fd->Buffer + fd->Public.Position % SECTOR_SIZE, take);
        // printf("COPY: addrOut: %lu, addrSrc: %lu, num: %lu\r\n", u8DataOut, fd->Buffer + fd->Public.Position % SECTOR_SIZE, take);
        u8DataOut += take;
        fd->Public.Position += take;
        byteCount -= take;
        // printf("bytecount: %lu, take: %lu, leftInBuffer: %lu, position: %lu, size: %lu\r\n", byteCount, take, leftInBuffer, fd->Public.Position, fd->Public.Size);

        if (leftInBuffer == take)
        {
            if (fd->Public.Handle == ROOT_DIRECTORY_HANDLE)
            {
                ++fd->CurrentCluster;

                if (!MBR_ReadSectors(disk, fd->CurrentCluster, 1, fd->Buffer))
                {
                    printf("FAT: read error!\r\n");
                    break;
                }
            }
            else
            {
                if (++fd->CurrentSectorInCluster >= g_Data->BS.BootSector.SectorsPerCluster)
                {
                    fd->CurrentSectorInCluster = 0;
                    fd->CurrentCluster = FAT_NextCluster(disk, fd->CurrentCluster);
                }

                if (fd->CurrentCluster >= 0xFFFFFFF8)
                {
                    fd->Public.Size = fd->Public.Position;
                    break;
                }

                if (!MBR_ReadSectors(disk, FAT_ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster, 1, fd->Buffer))
                {
                    printf("FAT: read error!\r\n");
                    break;
                }
            }
        }
    }

    return u8DataOut - (uint8_t*)dataOut;
}

bool FAT_ReadEntry(DISK* disk, FAT_File* file, FAT_DirectoryEntry* dirEntry)
{
    return FAT_Read(disk, file, sizeof(FAT_DirectoryEntry), dirEntry) == sizeof(FAT_DirectoryEntry);
}

void FAT_Seek(DISK* disk, FAT_File* file, uint32_t Position)
{
    FAT_FileData* fd = &g_Data->OpenedFiles[file->Handle];
    file->Position = Position;
    uint32_t clusterIndex = Position / (g_Data->BS.BootSector.SectorsPerCluster * g_Data->BS.BootSector.BytesPerSector);
    uint32_t cluster = fd->FirstCluster;
    for(uint32_t i = 0; i < clusterIndex; i++)
    {
        cluster = FAT_NextCluster(disk, cluster);
        if(cluster >= 0x0FFFFFF8) {
            printf("[FAT SEEK] [WARN]: Cluster is too high\r\n");
            return;
        }
    }

    fd->CurrentCluster = cluster;
    uint32_t clusterOff = Position % (g_Data->BS.BootSector.SectorsPerCluster * g_Data->BS.BootSector.BytesPerSector);
    uint32_t sector = clusterOff / g_Data->BS.BootSector.BytesPerSector;
    uint32_t lba = FAT_ClusterToLba(cluster) + sector;
    if(!MBR_ReadSectors(disk, lba, 1, fd->Buffer)) {
        printf("[FAT SEEK] [ERROR]: Failed to read from diskr\n\n");
        return;
    }
    fd->CurrentSectorInCluster = lba % g_Data->BS.BootSector.SectorsPerCluster;
}

void FAT_ResetPos(DISK* disk, FAT_File* file)
{
    FAT_FileData* fd = &g_Data->OpenedFiles[file->Handle];
    file->Position = 0;
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;
    if(!MBR_ReadSectors(disk, FAT_ClusterToLba(fd->FirstCluster), 1, fd->Buffer)) {
        printf("[FAT SEEK] [ERROR]: Failed to read from disk\n\n");
    }
}

void FAT_Close(FAT_File* file)
{
    if (file->Handle == ROOT_DIRECTORY_HANDLE)
    {
        file->Position = 0;
        g_Data->RootDirectory.CurrentCluster = g_Data->RootDirectory.FirstCluster;
    }
    else
    {
        g_Data->OpenedFiles[file->Handle].Opened = false;
    }
}

void FAT_GetShortName(const char* fileName, char shortName[12])
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

bool FAT_FindFile(DISK* disk, FAT_File* file, const char* name, FAT_DirectoryEntry* entryOut)
{
    char shortName[12];
    FAT_DirectoryEntry entry;

    FAT_GetShortName(name, shortName);

    while (FAT_ReadEntry(disk, file, &entry))
    {
        if (memcmp(shortName, entry.Name, 11) == 0)
        {
            *entryOut = entry;
            return true;
        }        
    }
    
    return false;
}

FAT_File* FAT_Open(DISK* disk, const char* path)
{
    char name[MAX_PATH_SIZE];

    if (path[0] == '/')
        path++;

    FAT_File* current = &g_Data->RootDirectory.Public;

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
        
        FAT_DirectoryEntry entry;
        if (FAT_FindFile(disk, current, name, &entry))
        {
            FAT_Close(current);

            if (!isLast && (entry.Attributes & FAT_ATTRIBUTE_DIRECTORY) == 0)
            {
                printf("FAT: %s not a directory\r\n", name);
                return NULL;
            }

            current = FAT_OpenEntry(disk, &entry);
        }
        else
        {
            FAT_Close(current);

            printf("FAT: %s not found\r\n", name);
            return NULL;
        }
    }

    return current;
}
