#include <fat32.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <algorithm>

using namespace FAT32::FAT;

bool FAT::ReadBootSector()
{
    return gpt->ReadSectors(0, 1, &data->BS.bootSectorBytes);
}

bool FAT::ReadFAT(m_uint32_t lbaIndex)
{
    return gpt->ReadSectors(fatData.FatStartLba + lbaIndex, FAT_CACHE_SIZE, data->FatCache);
}

bool FAT::Initialize(GPT::GPT *gpt)
{
    this->gpt = gpt;

    data = (Data *)malloc(sizeof(Data));
    if (!data)
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to allocate memory\n");
        return false;
    }

    bpb = &data->BS.bootSector.bpb;
    ebr = &data->BS.bootSector.ebr;

    if (!ReadBootSector())
        return false;

    fatData.TotalSectors = (bpb->totalSectors16 == 0) ? bpb->totalSectors32 : (m_uint32_t)bpb->totalSectors16;
    fatData.DataSectionLba = bpb->ReservedSectors + (bpb->FATCount * ebr->SectorsPerFAT);
    fatData.FatStartLba = bpb->ReservedSectors;
    fatData.TotalClusters = (fatData.TotalSectors - bpb->ReservedSectors - (bpb->FATCount * ebr->SectorsPerFAT)) / bpb->SectorsPerCluster;
    data->FatCacheSector = 0xFFFFFFFF;

    data->rootDirectory.Size = 0;
    data->rootDirectory.Position = 0;
    data->rootDirectory.IsDirectory = true;
    data->rootDirectory.FirstCluster = ebr->RootDirCluster;
    data->rootDirectory.CurrentCluster = ebr->RootDirCluster;
    data->rootDirectory.CurrentSectorInCluster = 0;

    return true;
}

void FAT::InitializeMin(GPT::GPT *gpt)
{
    this->gpt = gpt;
}

bool FAT::mkfs(MKFSDesc *desc)
{
    m_uint32_t totalSectors = gpt->GetPartitionSectorCount();
    if (desc->BytesPerSector == 0)
        desc->BytesPerSector = 512;
    if (desc->HeadCount == 0)
        desc->HeadCount = 255;
    if (desc->mediaDescType == 0)
        desc->mediaDescType = 0xF8;
    if (desc->SectorsPerCluster == 0)
    {
        if (totalSectors < 532480)
            desc->SectorsPerCluster = 1;
        else if (totalSectors <= 16777216)
            desc->SectorsPerCluster = 8;
        else if (totalSectors <= 33554432)
            desc->SectorsPerCluster = 16;
        else if (totalSectors <= 67108864)
            desc->SectorsPerCluster = 32;
        else if (totalSectors <= 134217728)
            desc->SectorsPerCluster = 64;
        else
            desc->SectorsPerCluster = 128;
    }
    if (desc->SectorsPerTrack == 0)
        desc->SectorsPerTrack = 63;

    BootSector bootSector = {};
    memset(&bootSector, 0, SECTOR_SIZE);
    bootSector.bootSig = 0xAA55;
    bootSector.bpb.bootJumpInstruction[0] = 0x90;
    bootSector.bpb.bootJumpInstruction[1] = 0xEB;
    bootSector.bpb.bootJumpInstruction[2] = 0x57;
    bootSector.bpb.BytesPerSector = desc->BytesPerSector;
    bootSector.bpb.FATCount = 2;
    bootSector.bpb.HeadCount = desc->HeadCount;
    bootSector.bpb.HiddenSectors = gpt->GetPartitionStartLba();
    bootSector.bpb.mediaDescType = desc->mediaDescType;
    char oemStr[] = {'M', 'S', 'W', 'I', 'N', '4', '.', '1'};
    memcpy(bootSector.bpb.oem, oemStr, 8);
    bootSector.bpb.ReservedSectors = 32;
    bootSector.bpb.rootDirEntries = 0;
    bootSector.bpb.SectorsPerCluster = desc->SectorsPerCluster;
    bootSector.bpb.SectorsPerTrack = desc->SectorsPerTrack;
    bootSector.bpb.totalSectors16 = (totalSectors > 0xFFFF) ? 0 : (m_uint16_t)totalSectors;
    if (bootSector.bpb.totalSectors16 == 0)
        bootSector.bpb.totalSectors32 = totalSectors;
    bootSector.bpb._Reserved = 0;
    bootSector.ebr.backupBootSector = 6;
    bootSector.ebr.DriveNumber = desc->DriveNumber;
    bootSector.ebr.Flags = 0;
    bootSector.ebr.FSInfoSector = 1;
    bootSector.ebr.RootDirCluster = 2;
    bootSector.ebr.Signature = 0x29;
    char SysIdentStr[] = {'F', 'A', 'T', '3', '2', ' ', ' ', ' '};
    memcpy(bootSector.ebr.SystemIdentifier, SysIdentStr, 8);
    bootSector.ebr.VersionNumber = 0;
    bootSector.ebr.VolumeID = desc->VolumeID;
    memcpy(bootSector.ebr.VolumeLabel, desc->VolumeLabel, 11);
    memset(bootSector.ebr._Reserved, 0, 12);
    bootSector.ebr._Reserved0 = 0;
    m_uint32_t estDataSectors = totalSectors - bootSector.bpb.ReservedSectors;
    m_uint32_t estTotalClusters = estDataSectors / bootSector.bpb.SectorsPerCluster;
    bootSector.ebr.SectorsPerFAT = (estTotalClusters * 4 + desc->BytesPerSector - 1) / desc->BytesPerSector;
    m_uint32_t TotalClusters = (totalSectors - bootSector.bpb.ReservedSectors - (bootSector.bpb.FATCount * bootSector.ebr.SectorsPerFAT)) / bootSector.bpb.SectorsPerCluster;

    m_uint8_t bootCode[] = {
        0xBE, 0x71, 0x7C, 0xE8, 0x02, 0x00, 0xFA, 0xF4,
        0x50, 0xB4, 0x0E, 0x8A, 0x04, 0x3C, 0x00, 0x74,
        0x05, 0xCD, 0x10, 0x46, 0xEB, 0xF5, 0x58, 0xC3,
        0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20,
        0x6E, 0x6F, 0x74, 0x20, 0x61, 0x20, 0x62, 0x6F,
        0x6F, 0x74, 0x61, 0x62, 0x6C, 0x65, 0x20, 0x64,
        0x69, 0x73, 0x6B, 0x2C, 0x20, 0x70, 0x6C, 0x65,
        0x61, 0x73, 0x65, 0x20, 0x69, 0x6E, 0x73, 0x65,
        0x72, 0x74, 0x20, 0x61, 0x20, 0x62, 0x6F, 0x6F,
        0x74, 0x61, 0x62, 0x6C, 0x65, 0x20, 0x64, 0x69,
        0x73, 0x6B, 0x20, 0x61, 0x6E, 0x64, 0x20, 0x74,
        0x72, 0x79, 0x20, 0x61, 0x67, 0x61, 0x69, 0x6E,
        0x0D, 0x0A, 0x00
    };

    memcpy(bootSector.bootCode, bootCode, sizeof(bootCode));

    if (!gpt->WriteSectors(0, 1, &bootSector))
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to write to DISK\n");
        return false;
    }

    if (!gpt->WriteSectors(bootSector.ebr.backupBootSector, 1, &bootSector)) // backup boot sector
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to write to DISK\n");
        return false;
    }

    FSInfo fsinfo = {};
    fsinfo.LeadSignature = 0x41615252;
    memset(fsinfo._Reserved, 0, sizeof(fsinfo._Reserved));
    memset(fsinfo._Reserved0, 0, sizeof(fsinfo._Reserved0));
    fsinfo.StructSignature = 0x61417272;
    fsinfo.FreeCount = TotalClusters - 3;
    fsinfo.NextFree = 2;
    fsinfo.TrailSignature = 0xAA55;

    if (!gpt->WriteSectors(bootSector.ebr.FSInfoSector, 1, &fsinfo))
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to write to DISK\n");
        return false;
    }

    m_uint8_t buffer[SECTOR_SIZE];
    memset(buffer, 0, SECTOR_SIZE);

    for (size_t i = 0; i < bootSector.bpb.FATCount * bootSector.ebr.SectorsPerFAT; i++)
    {
        if (!gpt->WriteSectors(bootSector.bpb.ReservedSectors + i, 1, &buffer))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to write to DISK\n");
            return false;
        }
    }

    m_uint32_t *FAT = reinterpret_cast<m_uint32_t *>(buffer);

    FAT[0] = bootSector.bpb.mediaDescType | 0xFFFFFF00;
    FAT[1] = 0xFFFFFFFF;
    FAT[2] = 0x0FFFFFFF;

    if (!gpt->WriteSectors(bootSector.bpb.ReservedSectors, 1, &buffer))
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to write to DISK\n");
        return false;
    }

    if (!gpt->WriteSectors(bootSector.bpb.ReservedSectors + bootSector.ebr.SectorsPerFAT, 1, &buffer))
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to write to DISK\n");
        return false;
    }

    return true;
}

m_uint32_t FAT::ClusterToLba(m_uint32_t cluster)
{
    return fatData.DataSectionLba + (cluster - 2) * bpb->SectorsPerCluster;
}

m_uint32_t FAT::LbaToCluster(m_uint32_t lba)
{
    return ((lba - fatData.DataSectionLba) / bpb->SectorsPerCluster) + 2;
}

m_uint32_t FAT::ReadFATEntry(m_uint32_t cluster)
{
    m_uint32_t entryOffset = cluster * 4;
    m_uint32_t sectorIndex = entryOffset / SECTOR_SIZE;
    m_uint32_t offsetInSector = entryOffset % SECTOR_SIZE;

    m_uint32_t sectorStart = sectorIndex - (sectorIndex % FAT_CACHE_SIZE);

    if (data->FatCacheSector != sectorStart)
    {
        if (!ReadFAT(sectorStart))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to read FAT sector %lu\n", sectorStart);
            return 0xFFFFFFFF;
        }
        data->FatCacheSector = sectorStart;
    }

    m_uint32_t offsetInCache = (sectorIndex - sectorStart) * SECTOR_SIZE + offsetInSector;
    m_uint32_t value = *(m_uint32_t *)&data->FatCache[offsetInCache];
    return value & 0x0FFFFFFF;
}

bool FAT::WriteFATEntry(m_uint32_t cluster, m_uint32_t value)
{
    m_uint32_t entryOffset = cluster * 4;
    m_uint32_t sectorIndex = entryOffset / SECTOR_SIZE;
    m_uint32_t offsetInSector = entryOffset % SECTOR_SIZE;

    m_uint32_t sectorStart = sectorIndex - (sectorIndex % FAT_CACHE_SIZE);

    if (data->FatCacheSector != sectorStart)
    {
        if (!ReadFAT(sectorStart))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to read FAT sector %lu\n", sectorStart);
            return false;
        }
        data->FatCacheSector = sectorStart;
    }

    m_uint32_t offsetInCache = (sectorIndex - sectorStart) * SECTOR_SIZE + offsetInSector;
    *(m_uint32_t *)&data->FatCache[offsetInCache] = value & 0x0FFFFFFF;

    for (m_uint32_t i = 0; i < bpb->FATCount; i++)
    {
        m_uint32_t fatSector = fatData.FatStartLba + i * ebr->SectorsPerFAT + sectorStart;
        if (!gpt->WriteSectors(fatSector, FAT_CACHE_SIZE, data->FatCache))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to write to DISK\n");
            return false;
        }
    }

    return true;
}

bool FAT::NextCluster(m_uint32_t &cluster)
{
    m_uint32_t next = ReadFATEntry(cluster);
    if (next >= 0x0FFFFFF8 || next == 0xFFFFFFFF)
        return false;
    cluster = next;
    return true;
}

bool FAT::AssembleLFN(LFNEntry *entries, int count, char *outName)
{
    int pos = 0;
    for (int i = count - 1; i >= 0; --i)
    {
        for (int j = 0; j < 5; j++)
        {
            if (entries[i].Entry1[j] == 0x0000)
                goto done; // I DON'T CARE IF YOU JUDGE ME ABOUT THIS
            outName[pos++] = (char)entries[i].Entry1[j];
        }
        for (int j = 0; j < 6; j++)
        {
            if (entries[i].Entry2[j] == 0x0000)
                goto done; // I DON'T CARE IF YOU JUDGE ME ABOUT THIS
            outName[pos++] = (char)entries[i].Entry2[j];
        }
        for (int j = 0; j < 2; j++)
        {
            if (entries[i].Entry3[j] == 0x0000)
                goto done; // I DON'T CARE IF YOU JUDGE ME ABOUT THIS
            outName[pos++] = (char)entries[i].Entry3[j];
        }
    }
done:
    outName[pos] = '\0';
    return true;
}

bool FAT::ReadEntry(File *dir, LFNDirectoryEntry *entryOut)
{
    if (!entryOut)
        return false;
    *entryOut = {};
    m_uint8_t buffer[SECTOR_SIZE];
    LFNEntry lfnEntries[FAT_MAX_LFN_ENTRIES];
    int lfnIndex = 0;

    while (true)
    {
        m_uint32_t entryIndex = dir->Position / sizeof(DirectoryEntry);
        m_uint32_t sectorOffset = (entryIndex * sizeof(DirectoryEntry)) / SECTOR_SIZE;
        m_uint32_t offsetInSector = (entryIndex * sizeof(DirectoryEntry)) % SECTOR_SIZE;

        m_uint32_t lba = ClusterToLba(dir->CurrentCluster) + sectorOffset;

        if (!gpt->ReadSectors(lba, 1, buffer))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to read DISK\n");
            return false;
        }

        DirectoryEntry *entries = reinterpret_cast<DirectoryEntry *>(buffer);
        DirectoryEntry *entry = &entries[(offsetInSector / sizeof(DirectoryEntry))];

        dir->Position += sizeof(DirectoryEntry);
        dir->CurrentSectorInCluster = (dir->Position % (bpb->SectorsPerCluster * SECTOR_SIZE)) / SECTOR_SIZE;
        if (dir->CurrentSectorInCluster >= bpb->SectorsPerCluster)
        {
            dir->CurrentSectorInCluster = 0;
            if (!NextCluster(dir->CurrentCluster))
            {
                fprintf(stderr, "[FAT32] [ERROR]: No more clusters are present\n");
                return false;
            }
        }

        if (entry->Name[0] == 0x00)
            return false;
        if (entry->Name[0] == 0xE5)
            continue;

        if ((entry->Attribs & 0x0F) == 0x0F)
        {
            if (lfnIndex < FAT_MAX_LFN_ENTRIES)
                lfnEntries[lfnIndex++] = *(LFNEntry *)entry;
            else
            {
                fprintf(stderr, "[FAT32] [ERROR]: Too many LFN entries\n");
                return false;
            }
            continue;
        }

        entryOut->SFNEntry = *entry;
        entryOut->LFN[0] = '\0';

        if (lfnIndex > 0)
        {
            if (!AssembleLFN(lfnEntries, lfnIndex, entryOut->LFN))
            {
                fprintf(stderr, "[FAT32] [ERROR]: Failed to assemble LFN\n");
                return false;
            }
            entryOut->IsLFN = true;
        }

        entryOut->LFNEntryCount = lfnIndex;
        return true;
    }
}

void FAT::GetShortName(const char *name, char shortName[11])
{
    memset(shortName, ' ', 11);

    if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
    {
        shortName[0] = '.';
        if (name[1] == '.')
            shortName[1] = '.';
        return;
    }

    const char *ext = strchr(name, '.');
    if (!ext)
        ext = name + 10;

    for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
        shortName[i] = toupper(name[i]);

    if (ext != name + 10)
        for (int i = 0; i < 3 && ext[i + 1]; i++)
            shortName[8 + i] = toupper(ext[i + 1]);
}

bool FAT::FindFile(LPCSTR path, LFNDirectoryEntry *entryOut)
{
    File temp = data->rootDirectory;
    File *current = &temp;
    LFNDirectoryEntry entry;

    if (path[0] == '/' || path[0] == '\\')
        path++;

    while (*path)
    {
        LPCSTR delim = strpbrk(path, "/\\");
        size_t len = delim ? (size_t)(delim - path) : strlen(path);

        char nameBuffer[FAT_MAX_FILE_NAME_SIZE] = {};
        strncpy(nameBuffer, path, len);
        nameBuffer[len] = '\0';

        char shortName[11] = {};
        GetShortName(nameBuffer, shortName);

        bool found = false;
        ResetPos(current);

        while (ReadEntry(current, &entry))
        {
            if (entry.IsLFN)
            {
                if (strcasecmp(entry.LFN, nameBuffer) == 0)
                {
                    found = true;
                    break;
                }
            }
            else
            {
                if (strncasecmp((const char *)entry.SFNEntry.Name, shortName, 11) == 0)
                {
                    found = true;
                    break;
                }
            }
        }

        if (!found)
            return false;

        if (delim)
        {
            if (!(entry.SFNEntry.Attribs & FileAttribs::DIRECTORY))
                return false;

            current->FirstCluster = ((m_uint32_t)entry.SFNEntry.FirstClusterHigh << 16) | entry.SFNEntry.FirstClusterLow;
            current->CurrentCluster = current->FirstCluster;
            current->Position = 0;
            current->IsDirectory = true;

            path = delim + 1;
        }
        else
        {
            *entryOut = entry;
            return true;
        }
    }
    return false;
}

bool FAT::FindFileInDir(File *dir, LPCSTR name, LFNDirectoryEntry *entryOut)
{
    if (!dir || !name || !entryOut)
        return false;

    if (!IsValidEntryName(name))
    {
        fprintf(stderr, "[FAT32] [ERROR]: Unsupported name %s\n", name);
        return false;
    }

    size_t len = strlen(name);
    while (len > 0 && (name[len - 1] == '/' || name[len - 1] == '\\'))
        len--;
    if (len == 0)
        return false;

    char nameBuffer[FAT_MAX_FILE_NAME_SIZE];
    strncpy(nameBuffer, name, len);
    nameBuffer[len] = '\0';

    char shortName[11];
    GetShortName(nameBuffer, shortName);

    ResetPos(dir);

    LFNDirectoryEntry entry;
    while (ReadEntry(dir, &entry))
    {
        if (entry.IsLFN)
        {
            if (strcasecmp(entry.LFN, nameBuffer) == 0)
            {
                *entryOut = entry;
                return true;
            }
        }
        else
        {
            if (strncasecmp((LPCSTR)entry.SFNEntry.Name, shortName, 11) == 0)
            {
                *entryOut = entry;
                return true;
            }
        }
    }

    return false;
}

bool FAT::OpenFile(LPCSTR path, File *file)
{
    if ((path[0] == '/' || path[0] == '\\') && path[1] == '\0')
    {
        memcpy(file, &data->rootDirectory, sizeof(File));
        return true;
    }
    LFNDirectoryEntry entry = {};
    if (!FindFile(path, &entry))
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to find file %s\n", path);
        return false;
    }
    const DirectoryEntry *sfn = &entry.SFNEntry;
    file->FirstCluster = ((m_uint32_t)sfn->FirstClusterHigh << 16) | sfn->FirstClusterLow;
    ResetPos(file);
    file->Size = sfn->FileSize;
    file->IsDirectory = (sfn->Attribs & FileAttribs::DIRECTORY) != 0;
    file->fileEntry = entry;
    return true;
}

bool FAT::OpenFileInDir(File *dir, LPCSTR name, File *file)
{

    LFNDirectoryEntry entry = {};
    if (!FindFileInDir(dir, name, &entry))
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to find file %s\n", name);
        return false;
    }

    const DirectoryEntry *sfn = &entry.SFNEntry;
    file->FirstCluster = ((m_uint32_t)sfn->FirstClusterHigh << 16) | sfn->FirstClusterLow;
    ResetPos(file);
    file->Size = sfn->FileSize;
    file->IsDirectory = (sfn->Attribs & FileAttribs::DIRECTORY) != 0;
    file->fileEntry = entry;
    return true;
}

m_uint32_t FAT::ReadFile(File *file, void *buffer, m_uint32_t size)
{
    if (file->IsDirectory)
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to read file: Is a directory\n");
        return 0;
    }

    if (file->CurrentCluster == 0xFFFFFFFF || file->Position >= file->Size)
    {
        return 0;
    }

    m_uint32_t read = 0;
    m_uint8_t *buf = static_cast<m_uint8_t *>(buffer);
    m_uint32_t bytesLeft = size;

    while (bytesLeft > 0 && file->CurrentCluster != 0xFFFFFFFF && file->Position < file->Size)
    {
        m_uint32_t lba = ClusterToLba(file->CurrentCluster) + file->CurrentSectorInCluster;
        m_uint8_t sector[SECTOR_SIZE];

        if (!gpt->ReadSectors(lba, 1, sector))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to read from DISK\n");
            return read;
        }

        m_uint32_t offsetInSector = file->Position % bpb->BytesPerSector;
        m_uint32_t bytesInSector = bpb->BytesPerSector - offsetInSector;
        m_uint32_t bytesToCopy = (bytesLeft < bytesInSector) ? bytesLeft : bytesInSector;
        m_uint32_t left = file->Size - file->Position;
        if (bytesToCopy > left)
            bytesToCopy = left;

        memcpy(buf, sector + offsetInSector, bytesToCopy);
        buf += bytesToCopy;
        file->Position += bytesToCopy;
        bytesLeft -= bytesToCopy;
        read += bytesToCopy;

        file->CurrentSectorInCluster++;
        if (file->CurrentSectorInCluster >= bpb->SectorsPerCluster)
        {
            file->CurrentSectorInCluster = 0;
            if (!NextCluster(file->CurrentCluster))
            {
                file->CurrentCluster = 0xFFFFFFFF;
                break;
            }
        }
    }

    return read;
}

void FAT::Seek(File *file, m_uint32_t pos)
{
    file->Position = pos;
    file->CurrentCluster = file->FirstCluster;
    for (m_uint32_t i = 0; i < (pos / (bpb->SectorsPerCluster * bpb->BytesPerSector)); i++)
    {
        if (!NextCluster(file->CurrentCluster))
        {
            file->CurrentCluster = 0xFFFFFFFF;
            return;
        }
    }

    file->CurrentSectorInCluster = pos % (bpb->SectorsPerCluster * bpb->BytesPerSector) / bpb->BytesPerSector;
}

void FAT::ResetPos(File *file)
{
    file->Position = 0;
    file->CurrentCluster = file->FirstCluster;
    file->CurrentSectorInCluster = 0;
}

bool FAT::CreateEntry(LPCSTR path, bool isDirectory, File *fileOut)
{
    if (!path || *path == '\0' || !fileOut)
        return false;

    if (path[0] == '/' || path[0] == '\\')
        path++;

    File current = data->rootDirectory;

    char pathCopy[FAT_MAX_FILE_NAME_SIZE];
    strncpy(pathCopy, path, sizeof(pathCopy) - 1);

    char *segment = strtok(pathCopy, "/\\");
    char *nextSegment = nullptr;

    while (segment)
    {
        nextSegment = strtok(nullptr, "/\\"); // nullptr is passed because strtok saves the string passed in the latest call with a non-nullptr first arg

        char shortName[11];

        GetShortName(segment, shortName);

        bool IsLast = (nextSegment == nullptr);

        LFNDirectoryEntry entry;
        bool found = FindFileInDir(&current, segment, &entry);

        if (!found)
        {
            if (!IsLast)
            {
                fprintf(stderr, "[FAT32] [ERROR]: Directory %s doesn't exist\n", segment);
                return false;
            }

            if (!IsValidEntryName(segment))
            {
                fprintf(stderr, "[FAT32] [ERROR]: Invalid name %s\n", segment);
                return false;
            }

            LFNDirectoryEntry newEntry = {};
            memcpy(newEntry.SFNEntry.Name, shortName, 11);
            newEntry.SFNEntry.Attribs = isDirectory ? FileAttribs::DIRECTORY : FileAttribs::ARCHIVE;
            newEntry.SFNEntry.FirstClusterHigh = 0;
            newEntry.SFNEntry.FirstClusterLow = 0;
            newEntry.SFNEntry.FileSize = 0;

            if (!CreateDirectoryEntry(&current, segment, &newEntry))
            {
                fprintf(stderr, "[FAT32] [ERROR]: Failed to create directory entry\n");
                return false;
            }

            if (!OpenFile(path, fileOut))
            {
                fprintf(stderr, "[FAT32] [ERROR]: Failed to open %s %s\n", isDirectory ? "directory" : "file", path);
                return false;
            }

            if (isDirectory)
            {
                LFNDirectoryEntry entry = {};
                entry.IsLFN = false;
                entry.LFN[0] = '\0';
                entry.SFNEntry.Attribs = FileAttribs::DIRECTORY;
                entry.SFNEntry.FileSize = 0;
                memcpy(entry.SFNEntry.Name, ".          ", 11);
                entry.SFNEntry._Reserved = 0;
                entry.SFNEntry.FirstClusterHigh = newEntry.SFNEntry.FirstClusterHigh;
                entry.SFNEntry.FirstClusterLow = newEntry.SFNEntry.FirstClusterLow;

                if (!CreateDirectoryEntry(fileOut, ".", &entry))
                {
                    fprintf(stderr, "[FAT32] [ERROR]: Failed to create directory entry\n");
                    return false;
                }

                memcpy(entry.SFNEntry.Name, "..         ", 11);
                entry.SFNEntry.FirstClusterLow = (m_uint16_t)(current.FirstCluster & 0xFFFF);
                entry.SFNEntry.FirstClusterHigh = (m_uint16_t)((current.FirstCluster >> 16) & 0xFFFF);

                if (!CreateDirectoryEntry(fileOut, "..", &entry))
                {
                    fprintf(stderr, "[FAT32] [ERROR]: Failed to create directory entry\n");
                    return false;
                }
            }

            return true;
        }
        else
        {
            if (IsLast)
            {
                fprintf(stderr, "[FAT32] [ERROR]: File / Directory %s already exists\n", segment);
                return false;
            }

            if (!(entry.SFNEntry.Attribs & FileAttribs::DIRECTORY))
            {
                fprintf(stderr, "[FAT32] [ERROR]: %s is not a directory\n", segment);
                return false;
            }

            current.FirstCluster = ((m_uint32_t)entry.SFNEntry.FirstClusterHigh << 16) | entry.SFNEntry.FirstClusterLow;
            current.CurrentCluster = current.FirstCluster;
            current.Position = 0;
            current.IsDirectory = true;
        }

        segment = nextSegment;
    }

    return false;
}

bool FAT::CreateDirectoryEntry(File *dir, LPCSTR name, LFNDirectoryEntry *entry)
{
    if (!dir || !name || !entry)
        return false;

    const m_uint16_t entriesPerSector = SECTOR_SIZE / sizeof(DirectoryEntry);
    m_uint8_t buffer[SECTOR_SIZE];

    m_uint32_t currentCluster = (dir->FirstCluster == 0) ? data->rootDirectory.FirstCluster : dir->FirstCluster;
    if (currentCluster == 0)
    {
        fprintf(stderr, "[FAT32] [ERROR]: Directory has no clusters allocated\n");
        return false;
    }

    if (entry->SFNEntry.Attribs & FileAttribs::DIRECTORY)
    {
        m_uint32_t newDirCluster = AllocateCluster();
        if (newDirCluster == 0)
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to allocate cluster for new directory\n");
            return false;
        }

        entry->SFNEntry.FirstClusterHigh = (newDirCluster >> 16) & 0xFFFF;
        entry->SFNEntry.FirstClusterLow = newDirCluster & 0xFFFF;
    }

    if (IsValidShortName(name))
    {
        while (true)
        {
            for (m_uint32_t sectorOffset = 0; sectorOffset < bpb->SectorsPerCluster; sectorOffset++)
            {
                m_uint32_t lba = ClusterToLba(currentCluster) + sectorOffset;
                if (!gpt->ReadSectors(lba, 1, buffer))
                {
                    fprintf(stderr, "[FAT32] [ERROR]: Failed to read from DISK\n");
                    return false;
                }

                DirectoryEntry *entries = reinterpret_cast<DirectoryEntry *>(buffer);

                for (size_t i = 0; i < entriesPerSector; i++)
                {
                    if (entries[i].Name[0] == 0x00 || entries[i].Name[0] == 0xE5)
                    {
                        memcpy(&entries[i], &entry->SFNEntry, sizeof(DirectoryEntry));
                        if (!gpt->WriteSectors(lba, 1, buffer))
                        {
                            fprintf(stderr, "[FAT32] [ERROR]: Failed to write to DISK\n");
                            return false;
                        }
                        return true;
                    }
                }
            }

            m_uint32_t nextCluster = currentCluster;
            if (!NextCluster(nextCluster))
            {
                m_uint32_t newCluster = AllocateCluster();
                if (newCluster == 0)
                {
                    fprintf(stderr, "[FAT32] [ERROR]: Failed to allocate new cluster\n");
                    return false;
                }

                if (!WriteFATEntry(currentCluster, newCluster) ||
                    !WriteFATEntry(newCluster, 0x0FFFFFFF))
                {
                    fprintf(stderr, "[FAT32] [ERROR]: Failed to link new FAT entries\n");
                    return false;
                }

                ZeroCluster(newCluster);
                currentCluster = newCluster;
            }
            else
            {
                currentCluster = nextCluster;
            }
        }
    }
    else
    {
        wchar wname[FAT_MAX_FILE_NAME_SIZE];
        int wlen = ConvertToUTF16(name, wname, FAT_MAX_FILE_NAME_SIZE - 1);
        int lfnCount = (wlen + 12) / 13;
        if (lfnCount > 20)
        {
            fprintf(stderr, "[FAT32] [ERROR]: Filename is too long, max 256 chars\n");
            return false;
        }

        while (true)
        {
            for (m_uint32_t sectorOffset = 0; sectorOffset < bpb->SectorsPerCluster; sectorOffset++)
            {
                m_uint32_t lba = ClusterToLba(currentCluster) + sectorOffset;
                if (!gpt->ReadSectors(lba, 1, buffer))
                {
                    fprintf(stderr, "[FAT32] [ERROR]: Failed to read from DISK\n");
                    return false;
                }

                DirectoryEntry *entries = reinterpret_cast<DirectoryEntry *>(buffer);

                for (int i = 0; i <= entriesPerSector - (lfnCount + 1); i++)
                {
                    bool blockFree = true;
                    for (size_t j = 0; j < (size_t)(lfnCount + 1); j++)
                    {
                        if (entries[i + j].Name[0] != 0x00 && entries[i + j].Name[0] != 0xE5)
                        {
                            blockFree = false;
                            break;
                        }
                    }
                    if (!blockFree)
                        continue;

                    for (int l = 0; l < lfnCount; l++)
                    {
                        int seqNum = l + 1;
                        if (l == lfnCount - 1)
                            seqNum |= 0x40;

                        int entryIndex = i + (lfnCount - 1 - l);
                        LFNEntry *lfn = reinterpret_cast<LFNEntry *>(&entries[entryIndex]);

                        lfn->Order = seqNum;
                        lfn->Attrib = 0x0F;
                        lfn->EntryType = 0;
                        lfn->Checksum = CalculateShortNameChecksum(entry->SFNEntry.Name);
                        lfn->_Reserved = 0;

                        int offset = l * 13;
                        for (int k = 0; k < 5; k++)
                            lfn->Entry1[k] = (offset + k < wlen) ? wname[offset + k] : 0x0000;
                        for (int k = 0; k < 6; k++)
                            lfn->Entry2[k] = (offset + 5 + k < wlen) ? wname[offset + 5 + k] : 0x0000;
                        for (int k = 0; k < 2; k++)
                            lfn->Entry3[k] = (offset + 11 + k < wlen) ? wname[offset + 11 + k] : 0x0000;
                    }

                    memcpy(&entries[i + lfnCount], &entry->SFNEntry, sizeof(DirectoryEntry));

                    if (!gpt->WriteSectors(lba, 1, buffer))
                    {
                        fprintf(stderr, "[FAT32] [ERROR]: Failed to write LFN entries to DISK\n");
                        return false;
                    }
                    return true;
                }
            }

            m_uint32_t nextCluster = currentCluster;
            if (!NextCluster(nextCluster))
            {
                m_uint32_t newCluster = AllocateCluster();
                if (newCluster == 0)
                {
                    fprintf(stderr, "[FAT32] [ERROR]: Failed to allocate new cluster\n");
                    return false;
                }

                if (!WriteFATEntry(currentCluster, newCluster) ||
                    !WriteFATEntry(newCluster, 0x0FFFFFFF))
                {
                    fprintf(stderr, "[FAT32] [ERROR]: Failed to link new FAT entries\n");
                    return false;
                }

                ZeroCluster(newCluster);
                currentCluster = newCluster;
            }
            else
            {
                currentCluster = nextCluster;
            }
        }
    }
}

m_uint32_t FAT::AllocateCluster()
{
    m_uint32_t totalClusters = LbaToCluster(fatData.TotalSectors - fatData.DataSectionLba);

    for (m_uint32_t cluster = 2; cluster < totalClusters; cluster++)
    {
        m_uint32_t entry = ReadFATEntry(cluster);
        if (entry == 0)
        {
            if (!WriteFATEntry(cluster, 0x0FFFFFFF))
            {
                fprintf(stderr, "[FAT32] [ERROR]: Failed to write FAT entry\n");
                return 0;
            }

            ZeroCluster(cluster);
            return cluster;
        }
    }

    return 0;
}

void FAT::ZeroCluster(m_uint32_t cluster)
{
    m_uint8_t buffer[SECTOR_SIZE];
    memset(buffer, 0, SECTOR_SIZE);

    m_uint32_t startLba = ClusterToLba(cluster);
    for (m_uint32_t sector = 0; sector < bpb->SectorsPerCluster; sector++)
    {
        if (!gpt->WriteSectors(startLba + sector, 1, buffer))
            fprintf(stderr, "[FAT32] [ERROR]: Failed to write to DISK\n");
    }
}

bool FAT::IsValidShortName(LPCSTR name)
{
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return true;

    LPCSTR validChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789$%'-_@~`!(){}^#&";

    int nameLen = 0;
    int extLen = 0;
    LPCSTR dot = strchr(name, '.');

    if (!dot)
        dot = name + strlen(name);

    for (LPCSTR p = name; p < dot && nameLen < 8; p++, nameLen++)
    {
        if (!strchr(validChars, toupper(*p)))
            return false;
        if (islower(*p))
            return false;
    }

    if (*dot == '.')
    {
        for (LPCSTR p = dot + 1; *p && extLen < 3; p++, extLen++)
        {
            if (!strchr(validChars, toupper(*p)))
                return false;
            if (islower(*p))
                return false;
        }
        if (*(dot + 1 + extLen) != '\0')
            return false;
    }
    else if (*dot != '\0')
        return false;

    return nameLen > 0;
}

m_uint8_t FAT::CalculateShortNameChecksum(m_uint8_t shortName[11])
{
    m_uint8_t sum = 0;

    for (int i = 0; i < 11; i++)
    {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1);
        sum += shortName[i];
    }

    return sum;
}

void FAT::GetTimeFormatted(m_uint16_t Time, LPSTR strOut)
{
    m_uint8_t Hour = (Time >> 11) & 0x1F;
    m_uint8_t Minute = (Time >> 5) & 0x3F;

    LPCSTR suffix = "AM";
    if (Hour >= 12)
        suffix = "PM";

    Hour %= 12;
    if (Hour == 0)
        Hour = 12;

    strOut[0] = '0' + (Hour / 10);
    strOut[1] = '0' + (Hour % 10);
    strOut[2] = ':';
    strOut[3] = '0' + (Minute / 10);
    strOut[4] = '0' + (Minute % 10);
    strOut[5] = ' ';
    strOut[6] = suffix[0];
    strOut[7] = suffix[1];
    strOut[8] = '\0';
}

void FAT::GetDateFormatted(m_uint16_t Date, LPSTR strOut)
{
    m_uint16_t year = ((Date >> 9) & 0x7F) + 1980;
    m_uint8_t month = (Date >> 5) & 0x0F;
    m_uint8_t day = Date & 0x1F;

    strOut[0] = '0' + ((year / 1000) % 10);
    strOut[1] = '0' + ((year / 100) % 10);
    strOut[2] = '0' + ((year / 10) % 10);
    strOut[3] = '0' + (year % 10);
    strOut[4] = '-';
    strOut[5] = '0' + (month / 10);
    strOut[6] = '0' + (month % 10);
    strOut[7] = '-';
    strOut[8] = '0' + (day / 10);
    strOut[9] = '0' + (day % 10);
    strOut[10] = '\0';
}

m_uint32_t FAT::GetBytesFree()
{
    m_uint32_t freeClusters = 0;

    for (m_uint32_t cluster = 2; cluster < fatData.TotalClusters + 2; cluster++)
        if (ReadFATEntry(cluster) == 0)
            freeClusters++;
    return freeClusters * bpb->SectorsPerCluster * bpb->BytesPerSector;
}

bool FAT::DeleteEntry(LPCSTR path, bool isDirectory)
{
    if (!path)
        return false;

    if (path[0] == '/' || path[0] == '\\')
        path++;

    File parentDir;

    if (strpbrk(path, "/\\") != nullptr)
    {
        char tmpPath[FAT_MAX_FILE_NAME_SIZE];
        strncpy(tmpPath, path, FAT_MAX_FILE_NAME_SIZE);
        tmpPath[FAT_MAX_FILE_NAME_SIZE - 1] = '\0';

        size_t len = strlen(path);

        for (int i = (int)len; i > 0; i--)
        {
            if (path[i - 1] == '/' || path[i - 1] == '\\')
            {
                tmpPath[i - 1] = '\0';
                break;
            }
            tmpPath[i - 1] = '\0';
        }

        if (!OpenFile(tmpPath, &parentDir))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to open directory %s\n", tmpPath);
            return false;
        }
    }
    else
        parentDir = data->rootDirectory;

    LFNDirectoryEntry fileEntry;
    if (!FindFile(path, &fileEntry))
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to find %s %s\n", isDirectory ? "directory" : "file", path);
        return false;
    }

    m_uint32_t cluster = ((m_uint32_t)fileEntry.SFNEntry.FirstClusterHigh << 16) | fileEntry.SFNEntry.FirstClusterLow;
    while (cluster >= 2 && cluster < 0x0FFFFFF8)
    {
        WriteFATEntry(cluster, 0x00000000);
        NextCluster(cluster);
    }

    ResetPos(&parentDir);

    m_uint8_t buffer[SECTOR_SIZE];

    m_uint8_t targetChecksum = CalculateShortNameChecksum(fileEntry.SFNEntry.Name);

    while (true)
    {
        m_uint32_t entryIndex = parentDir.Position / sizeof(DirectoryEntry);
        m_uint32_t sectorOffset = (entryIndex * sizeof(DirectoryEntry)) / SECTOR_SIZE;
        m_uint32_t offsetInSector = (entryIndex * sizeof(DirectoryEntry)) % SECTOR_SIZE;

        m_uint32_t lba = ClusterToLba(parentDir.CurrentCluster) + sectorOffset;

        if (!gpt->ReadSectors(lba, 1, &buffer))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to read from DISK\n");
            return false;
        }

        DirectoryEntry *entries = reinterpret_cast<DirectoryEntry *>(buffer);
        DirectoryEntry *entry = &entries[(offsetInSector / sizeof(DirectoryEntry))];

        if (entry->Name[0] == 0x00)
            break;

        bool deleteEntry = false;
        if ((entry->Attribs & FileAttribs::LFN) == FileAttribs::LFN)
        {
            LFNEntry *lfnEntry = reinterpret_cast<LFNEntry *>(entry);
            if (lfnEntry->Checksum == targetChecksum)
                deleteEntry = true;
        }
        else
        {
            if (memcmp(entry->Name, fileEntry.SFNEntry.Name, 11) == 0)
                deleteEntry = true;
        }

        if (deleteEntry)
        {
            entry->Name[0] = 0xE5;
            if (!gpt->WriteSectors(lba, 1, buffer))
            {
                fprintf(stderr, "[FAT32] [ERROR]: Failed to write to DISK\n");
                return false;
            }
        }

        parentDir.Position += sizeof(DirectoryEntry);

        parentDir.CurrentSectorInCluster = (parentDir.Position % (bpb->SectorsPerCluster * SECTOR_SIZE)) / SECTOR_SIZE;

        if (parentDir.CurrentSectorInCluster >= bpb->SectorsPerCluster)
        {
            parentDir.CurrentSectorInCluster = 0;
            if (!NextCluster(parentDir.CurrentCluster))
            {
                break;
            }
        }
    }

    return true;
}

bool FAT::DeleteDir(LPCSTR path)
{
    File dir;
    if (!OpenFile(path, &dir))
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to open directory %s\n", path);
        return false;
    }

    LFNDirectoryEntry entry;
    while (ReadEntry(&dir, &entry))
    {
        if (entry.SFNEntry.Name[0] == '.' || (entry.SFNEntry.Name[0] == '.' && entry.SFNEntry.Name[1] == '.'))
            continue;

        char childPath[FAT_MAX_PATH_SIZE];
        snprintf(childPath, sizeof(childPath), "%s/%s", path, entry.IsLFN ? entry.LFN : (LPCSTR)entry.SFNEntry.Name);

        if (entry.SFNEntry.Attribs & FileAttribs::DIRECTORY)
        {
            if (!DeleteDir(childPath))
                return false;
        }
        else if (!DeleteEntry(childPath, false))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to delete file %s\n", childPath);
            return false;
        }
    }

    if (!DeleteEntry(path, true))
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to delete directory %s\n", path);
        return false;
    }

    return true;
}

bool FAT::RenameEntry(LPCSTR path, LPCSTR newName, bool isDirectory, File *oldFile)
{
    if (!path || !newName || strlen(newName) == 0)
        return false;

    if (path[0] == '/' || path[0] == '\\')
        path++;

    File parentDir;
    if (strpbrk(path, "/\\") != nullptr)
    {
        char tmpPath[FAT_MAX_FILE_NAME_SIZE];
        strncpy(tmpPath, path, FAT_MAX_FILE_NAME_SIZE);
        tmpPath[FAT_MAX_FILE_NAME_SIZE - 1] = '\0';

        size_t len = strlen(path);

        for (int i = (int)len; i > 0; i--)
        {
            if (path[i - 1] == '/' || path[i - 1] == '\\')
            {
                tmpPath[i - 1] = '\0';
                break;
            }
            tmpPath[i - 1] = '\0';
        }

        if (!OpenFile(tmpPath, &parentDir))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to open directory %s\n", tmpPath);
            return false;
        }
    }
    else
        parentDir = data->rootDirectory;
    LFNDirectoryEntry oldEntry;
    if (!FindFile(path, &oldEntry))
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to find %s %s\n", isDirectory ? "directory" : "file", path);
        return false;
    }

    m_uint8_t oldChecksum = CalculateShortNameChecksum(oldEntry.SFNEntry.Name);

    ResetPos(&parentDir);
    m_uint8_t buffer[SECTOR_SIZE];

    while (true)
    {
        m_uint32_t entryIndex = parentDir.Position / sizeof(DirectoryEntry);
        m_uint32_t sectorOffset = (entryIndex * sizeof(DirectoryEntry)) / SECTOR_SIZE;
        m_uint32_t offsetInSector = (entryIndex * sizeof(DirectoryEntry)) % SECTOR_SIZE;

        m_uint32_t lba = ClusterToLba(parentDir.CurrentCluster) + sectorOffset;
        if (!gpt->ReadSectors(lba, 1, buffer))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to read from DISK\n");
            return false;
        }

        DirectoryEntry *entries = reinterpret_cast<DirectoryEntry *>(buffer);
        DirectoryEntry *entry = &entries[offsetInSector / sizeof(DirectoryEntry)];

        if (entry->Name[0] == 0x00)
            break;

        bool deleteEntry = false;
        if ((entry->Attribs & FileAttribs::LFN) == FileAttribs::LFN)
        {
            LFNEntry *lfnEntry = reinterpret_cast<LFNEntry *>(entry);
            if (lfnEntry->Checksum == oldChecksum)
                deleteEntry = true;
        }
        else if (memcmp(entry->Name, oldEntry.SFNEntry.Name, 11) == 0)
            deleteEntry = true;

        if (deleteEntry)
        {
            entry->Name[0] = 0xE5;
            if (!gpt->WriteSectors(lba, 1, buffer))
            {
                fprintf(stderr, "[FAT32] [ERROR]: Failed to write to DISK\n");
                return false;
            }
        }

        parentDir.Position += sizeof(DirectoryEntry);
        parentDir.CurrentSectorInCluster = (parentDir.Position % (bpb->SectorsPerCluster * SECTOR_SIZE)) / SECTOR_SIZE;

        if (parentDir.CurrentSectorInCluster >= bpb->SectorsPerCluster)
        {
            parentDir.CurrentSectorInCluster = 0;
            if (!NextCluster(parentDir.CurrentCluster))
                break;
        }
    }

    char shortName[12];
    GetShortName(newName, shortName);
    shortName[11] = '\0';

    bool NeedsLFN = !IsValidShortName(newName);

    m_uint8_t checksum = CalculateShortNameChecksum((m_uint8_t *)shortName);
    size_t nameLen = strlen(newName);

    size_t lfnCount = 0;

    DirectoryEntry entries[21];
    memset(entries, 0, sizeof(entries));

    if (NeedsLFN)
    {
        lfnCount = (nameLen + 12) / 13;
        for (size_t i = 0; i < lfnCount; i++)
        {
            LFNEntry *lfn = reinterpret_cast<LFNEntry *>(&entries[i]);
            m_uint8_t order = (m_uint8_t)(lfnCount - i);
            if (i == 0)
                order |= 0x40;
            lfn->Order = order;

            lfn->Attrib = FileAttribs::LFN;
            lfn->EntryType = 0;
            lfn->Checksum = checksum;
            lfn->_Reserved = 0;

            size_t baseIdx = (lfnCount - i - 1) * 13;

            for (size_t j = 0; j < 13; j++)
            {
                m_uint16_t ch = (baseIdx + j < nameLen) ? (m_uint8_t)newName[baseIdx + j] : 0x0000;

                if (j < 5)
                    lfn->Entry1[j] = ch;
                else if (j < 11)
                    lfn->Entry2[j - 5] = ch;
                else if (j < 13)
                    lfn->Entry3[j - 11] = ch;
            }
        }
    }

    DirectoryEntry *sfnEntry = &entries[lfnCount];
    memcpy(sfnEntry->Name, shortName, 11);
    sfnEntry->Attribs = isDirectory ? FileAttribs::DIRECTORY : FileAttribs::ARCHIVE;
    sfnEntry->FirstClusterHigh = oldEntry.SFNEntry.FirstClusterHigh;
    sfnEntry->FirstClusterLow = oldEntry.SFNEntry.FirstClusterLow;
    sfnEntry->FileSize = oldEntry.SFNEntry.FileSize;

    ResetPos(&parentDir);

    size_t totalEntries = lfnCount + 1;
    size_t entriesWritten = 0;

    while (entriesWritten < totalEntries)
    {
        m_uint32_t entryIndex = parentDir.Position / sizeof(DirectoryEntry);
        m_uint32_t sectorOffset = (entryIndex * sizeof(DirectoryEntry)) / SECTOR_SIZE;
        m_uint32_t offsetInSector = (entryIndex * sizeof(DirectoryEntry)) % SECTOR_SIZE;

        m_uint32_t lba = ClusterToLba(parentDir.CurrentCluster) + sectorOffset;
        if (!gpt->ReadSectors(lba, 1, buffer))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to read from DISK\n");
            return false;
        }

        DirectoryEntry *sectorEntries = reinterpret_cast<DirectoryEntry *>(buffer);

        while (offsetInSector < SECTOR_SIZE && entriesWritten < totalEntries)
        {
            DirectoryEntry *entry = &sectorEntries[offsetInSector / sizeof(DirectoryEntry)];

            if (entry->Name[0] == 0x00 || entry->Name[0] == 0xE5)
            {
                *entry = entries[entriesWritten];
                entriesWritten++;
            }
            offsetInSector += sizeof(DirectoryEntry);
            parentDir.Position += sizeof(DirectoryEntry);

            if (offsetInSector >= SECTOR_SIZE)
                break;
        }

        if (!gpt->WriteSectors(lba, 1, buffer))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to write to DISK\n");
            return false;
        }

        parentDir.CurrentSectorInCluster = (parentDir.Position % (bpb->SectorsPerCluster * SECTOR_SIZE)) / SECTOR_SIZE;
        if (parentDir.CurrentSectorInCluster >= bpb->SectorsPerCluster)
        {
            parentDir.CurrentSectorInCluster = 0;
            if (!NextCluster(parentDir.CurrentCluster))
            {
                fprintf(stderr, "[FAT32] [ERROR]: No free space for directory entries\n");
                return false;
            }
        }
    }

    if (!OpenFileInDir(&parentDir, newName, oldFile))
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to open new directory entry\n");
        return false;
    }

    return true;
}

bool FAT::IsValidEntryName(LPCSTR name)
{
    LPCSTR InvalidChars = "\"/\\*:<>?|+,:;=[]\n\r";
    size_t len = strlen(name);

    if (len == 0)
        return false;

    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return false;

    if (strpbrk(name, InvalidChars) != nullptr)
        return false;

    if (name[len - 1] == '.' || name[len - 1] == ' ')
        return false;

    return true;
}

bool FAT::CopyEntry(LPCSTR path, LPCSTR newPath, bool isDirectory, File *newFile)
{
    if (!path || !newPath || !newFile)
        return false;
    size_t pathLen = strlen(path), newPathLen = strlen(newPath);
    if (!pathLen || !newPathLen)
        return false;

    if (path[0] == '/' || path[0] == '\\')
        path++;
    if (newPath[0] == '/' || newPath[0] == '\\')
        newPath++;

    File origFile;
    if (!OpenFile(path, &origFile))
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to open %s %s\n", isDirectory ? "directory" : "file", path);
        return false;
    }

    if (!CreateEntry(newPath, isDirectory, newFile))
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to create %s %s\n", isDirectory ? "directory" : "file", newPath);
        return false;
    }

    if (origFile.FirstCluster == 0 && !isDirectory)
        return true; // the empty file has been already created, no cluster allocation needed

    m_uint32_t srcCluster = origFile.FirstCluster;
    m_uint32_t newFirstCluster = isDirectory ? newFile->FirstCluster : 0;
    m_uint32_t prevCluster = 0;

    if (isDirectory && newFirstCluster == 0)
        return false;

    if (isDirectory)
        prevCluster = newFirstCluster;

    while (srcCluster >= 2 && srcCluster < 0x0FFFFFF8)
    {
        m_uint32_t newCluster = AllocateCluster();
        if (newCluster == 0)
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to allocate new cluster\n");
            return false;
        }

        if (newFirstCluster == 0)
            newFirstCluster = newCluster;
        else
            WriteFATEntry(prevCluster, newCluster);

        prevCluster = newCluster;

        if (!NextCluster(srcCluster))
            break;
    }

    if (prevCluster != 0)
        WriteFATEntry(prevCluster, 0x0FFFFFFF);

    if (!isDirectory)
        newFile->FirstCluster = newFirstCluster;

    if (isDirectory)
    {
        srcCluster = origFile.FirstCluster;
        m_uint32_t destCluster = newFile->FirstCluster;
        const size_t clusterSize = bpb->BytesPerSector * bpb->SectorsPerCluster;
        m_uint8_t buffer[clusterSize];

        while (srcCluster >= 2 && srcCluster < 0x0FFFFFF8 && destCluster >= 2 && destCluster < 0x0FFFFFF8)
        {
            if (!gpt->ReadSectors(ClusterToLba(srcCluster), bpb->SectorsPerCluster, &buffer))
            {
                fprintf(stderr, "[FAT32] [ERROR]: Failed to read from DISK\n");
                return false;
            }

            if (!gpt->WriteSectors(ClusterToLba(destCluster), bpb->SectorsPerCluster, &buffer))
            {
                fprintf(stderr, "[FAT32] [ERROR]: Failed to write to DISK\n");
                return false;
            }
            if (!NextCluster(srcCluster))
                break; // EOC reached
            if (!NextCluster(destCluster))
                break; // EOC reached, but shouldn't happen
        }

        LFNDirectoryEntry entry;
        while (ReadEntry(&origFile, &entry))
        {
            LPCSTR name = entry.IsLFN ? entry.LFN : (LPCSTR)entry.SFNEntry.Name;
            if (name[0] == '.' || (name[0] == '.' && name[1] == '.'))
                continue;

            char childSrcPath[FAT_MAX_PATH_SIZE];
            char childDestPath[FAT_MAX_PATH_SIZE];
            snprintf(childSrcPath, sizeof(childSrcPath), "%s/%s", path, name);
            snprintf(childDestPath, sizeof(childDestPath), "%s/%s", newPath, name);

            bool entryIsDir = (entry.SFNEntry.Attribs & FileAttribs::DIRECTORY) != 0;

            File tmpNewFile;

            LFNDirectoryEntry entry;
            if (FindFile(childDestPath, &entry))
                return true; // entry already copied

            if (!CopyEntry(childSrcPath, childDestPath, entryIsDir, &tmpNewFile))
            {
                fprintf(stderr, "[FAT32] [ERROR]: Failed to copy %s %s\n", entryIsDir ? "directory" : "file", childSrcPath);
                return false;
            }
        }
    }
    else
    {
        srcCluster = origFile.FirstCluster;
        m_uint32_t destCluster = newFile->FirstCluster;
        const size_t clusterSize = bpb->BytesPerSector * bpb->SectorsPerCluster;
        m_uint8_t buffer[clusterSize];

        while (srcCluster >= 2 && srcCluster < 0x0FFFFFF8 && destCluster >= 2 && destCluster < 0x0FFFFFF8)
        {
            if (!gpt->ReadSectors(ClusterToLba(srcCluster), bpb->SectorsPerCluster, &buffer))
            {
                fprintf(stderr, "[FAT32] [ERROR]: Failed to read from DISK\n");
                return false;
            }

            if (!gpt->WriteSectors(ClusterToLba(destCluster), bpb->SectorsPerCluster, &buffer))
            {
                fprintf(stderr, "[FAT32] [ERROR]: Failed to write to DISK\n");
                return false;
            }
            if (!NextCluster(srcCluster))
                break; // EOC reached
            if (!NextCluster(destCluster))
                break; // EOC reached, but shouldn't happen
        }
    }

    File parentDir;

    if (strpbrk(newPath, "/\\") != nullptr)
    {
        char tmpPath[FAT_MAX_FILE_NAME_SIZE];
        strncpy(tmpPath, newPath, FAT_MAX_FILE_NAME_SIZE);
        tmpPath[FAT_MAX_FILE_NAME_SIZE - 1] = '\0';

        size_t len = strlen(newPath);

        for (int i = (int)len; i > 0; i--)
        {
            if (newPath[i - 1] == '/' || newPath[i - 1] == '\\')
            {
                tmpPath[i - 1] = '\0';
                break;
            }
            tmpPath[i - 1] = '\0';
        }

        if (!OpenFile(tmpPath, &parentDir))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to open directory %s\n", tmpPath);
            return false;
        }
    }
    else
        parentDir = data->rootDirectory;

    newFile->CurrentCluster = newFile->FirstCluster;

    LPCSTR fileName = newPath;
    LPCSTR lastSlash = strrchr(newPath, '/');
    if (!lastSlash)
        lastSlash = strrchr(newPath, '\\');
    if (lastSlash)
        fileName = lastSlash + 1;

    char shortName[12];
    GetShortName(fileName, shortName);
    shortName[11] = '\0';

    ResetPos(&parentDir);
    m_uint8_t buffer[SECTOR_SIZE];

    while (true)
    {
        m_uint32_t entryIndex = parentDir.Position / sizeof(DirectoryEntry);
        m_uint32_t sectorOffset = (entryIndex * sizeof(DirectoryEntry)) / SECTOR_SIZE;
        m_uint32_t offsetInSector = (entryIndex * sizeof(DirectoryEntry)) % SECTOR_SIZE;

        m_uint32_t lba = ClusterToLba(parentDir.CurrentCluster) + sectorOffset;
        if (!gpt->ReadSectors(lba, 1, buffer))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to read sector for update\n");
            return false;
        }

        DirectoryEntry *entries = reinterpret_cast<DirectoryEntry *>(buffer);
        DirectoryEntry *entry = &entries[offsetInSector / sizeof(DirectoryEntry)];

        if (entry->Name[0] == 0x00)
            break;

        if (memcmp(entry->Name, shortName, 11) == 0)
        {
            entry->FirstClusterLow = newFile->FirstCluster & 0xFFFF;
            entry->FirstClusterHigh = (newFile->FirstCluster >> 16) & 0xFFFF;
            entry->FileSize = isDirectory ? 0 : origFile.Size;

            newFile->fileEntry.SFNEntry = *entry;
            newFile->Size = entry->FileSize;

            if (!gpt->WriteSectors(lba, 1, buffer))
            {
                fprintf(stderr, "[FAT32] [ERROR]: Failed to write to DISK\n");
                return false;
            }
            break;
        }

        parentDir.Position += sizeof(DirectoryEntry);
        parentDir.CurrentSectorInCluster = (parentDir.Position % (bpb->SectorsPerCluster * SECTOR_SIZE)) / SECTOR_SIZE;

        if (parentDir.CurrentSectorInCluster >= bpb->SectorsPerCluster)
        {
            parentDir.CurrentSectorInCluster = 0;
            if (!NextCluster(parentDir.CurrentCluster))
            {
                fprintf(stderr, "[FAT32] [ERROR]: No more clusters left\n");
                return false;
            }
        }
    }

    return true;
}

bool FAT::MoveEntry(LPCSTR path, LPCSTR newPath, bool isDirectory, File *newFile)
{
    if (!CopyEntry(path, newPath, isDirectory, newFile))
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to copy %s %s\n", isDirectory ? "directory" : "file", path);
        return false;
    }

    if (isDirectory)
    {
        if (!DeleteDir(path))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to delete directory %s\n", path);
            return false;
        }
    }
    else
    {
        if (!DeleteEntry(path, false))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to delete file %s\n", path);
            return false;
        }
    }

    return true;
}

bool FAT::SetFileData(LPCSTR filePath, void *data, m_uint32_t count)
{
    if (!filePath || !data || count == 0)
        return false;

    if(filePath[0] == '/' || filePath[0] == '\\') 
        filePath++;

    File file;
    if (!OpenFile(filePath, &file))
    {
        fprintf(stderr, "[FAT32] [ERROR]: Failed to open file %s", filePath);
        return false;
    }

    m_uint32_t clustersNeeded = (count + (bpb->SectorsPerCluster * bpb->BytesPerSector) - 1) / (bpb->SectorsPerCluster * bpb->BytesPerSector);

    if (file.FirstCluster != 0)
    {
        FreeClusterChain(file.FirstCluster);
        file.FirstCluster = 0;
    }

    m_uint32_t prevCluster = 0, firstCluster = 0;

    for (m_uint32_t i = 0; i < clustersNeeded; i++)
    {
        m_uint32_t newCluster = AllocateCluster();
        if (newCluster == 0)
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to allocate more clusters\n");
            return false;
        }
        if (prevCluster != 0)
        {
            if (!WriteFATEntry(prevCluster, newCluster))
            {
                fprintf(stderr, "[FAT32] [ERROR]: Failed to write to FAT\n");
                return false;
            }
        }
        else
            firstCluster = newCluster;
        prevCluster = newCluster;
    }

    if (prevCluster != 0)
    {
        if (!WriteFATEntry(prevCluster, 0x0FFFFFF8))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to write to FAT\n");
            return false;
        }
    }

    m_uint8_t *dataPtr = (m_uint8_t *)data;
    m_uint32_t currentCluster = firstCluster;
    m_uint32_t bytesRemaining = count;

    for (m_uint32_t i = 0; i < clustersNeeded; i++)
    {
        m_uint32_t bytesToWrite = (bytesRemaining > (bpb->SectorsPerCluster * bpb->BytesPerSector)) ? (bpb->SectorsPerCluster * bpb->BytesPerSector) : bytesRemaining;

        if (!gpt->WriteSectors(ClusterToLba(currentCluster), bpb->SectorsPerCluster, dataPtr))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to write  to DISK\n");
            return false;
        }

        dataPtr += bytesToWrite;
        bytesRemaining -= bytesToWrite;

        if (i < clustersNeeded - 1)
            NextCluster(currentCluster);
    }

    file.FirstCluster = firstCluster;
    file.Size = count;

    // TODO: Update DirectoryEntry

    File parentDir;
    char parentName[FAT_MAX_PATH_SIZE], fileName[FAT_MAX_FILE_NAME_SIZE];

    LPCSTR lastSlash = strrchr(filePath, '/');
    if (!lastSlash)
        lastSlash = strrchr(filePath, '\\');

    if (lastSlash)
    {
        size_t parentLen = lastSlash - filePath;
        strncpy(parentName, filePath, parentLen);
        parentName[parentLen] = '\0';
        strncpy(fileName, lastSlash + 1, FAT_MAX_FILE_NAME_SIZE - 1);
        fileName[FAT_MAX_FILE_NAME_SIZE - 1] = '\0';

        if (!OpenFile(parentName, &parentDir))
        {
            fprintf(stderr, "[FAT32] [ERROR]: Failed to open parent dir\n");
            return false;
        }
    }
    else
    {
        strcpy(fileName, filePath);
        parentDir = this->data->rootDirectory;
    }

    LFNDirectoryEntry entry;
    bool found = false;
    while (ReadEntry(&parentDir, &entry))
    {
        if ((entry.SFNEntry.Attribs & FileAttribs::VOLUMEID) || (entry.SFNEntry.Attribs & FileAttribs::DIRECTORY))
            continue;
        if (entry.IsLFN)
        {
            if (strcmp(fileName, entry.LFN) == 0)
                found = true;
        }
        else
        {
            if (!IsValidShortName(fileName))
                continue;
            char shortName[11];
            GetShortName(fileName, shortName);
            if (memcmp(shortName, entry.SFNEntry.Name, 11) == 0)
                found = true;
        }

        if (found)
        {
            m_uint8_t buffer[SECTOR_SIZE];
            m_uint32_t lba = ClusterToLba(parentDir.CurrentCluster) + parentDir.CurrentSectorInCluster;
            if (!gpt->ReadSectors(lba, 1, &buffer))
            {
                fprintf(stderr, "[FAT32] [ERROR]: Failed to read from DISK\n");
                return false;
            }

            m_uint16_t offset = parentDir.Position % SECTOR_SIZE - sizeof(DirectoryEntry);
            DirectoryEntry *entry = reinterpret_cast<DirectoryEntry *>(&buffer[offset]);
            entry->FileSize = file.Size;
            entry->FirstClusterLow = (m_uint16_t)(file.FirstCluster & 0xFFFF);
            entry->FirstClusterHigh = (m_uint16_t)((file.FirstCluster >> 16) & 0xFFFF);

            if (!gpt->WriteSectors(lba, 1, &buffer))
            {
                fprintf(stderr, "[FAT32] [ERROR]: Failed to write to DISK\n");
                return false;
            }
            return true;
        }
    }

    return false;
}

void FAT::FreeClusterChain(m_uint32_t startCluster)
{
    m_uint32_t current = startCluster, next;

    for (m_uint32_t i = 0; i < fatData.TotalClusters; i++)
    {
        if (current < 2 || current > fatData.TotalClusters)
            break;

        next = current;
        NextCluster(next);

        WriteFATEntry(current, 0);

        if (next >= 0x0FFFFFF8)
            break;

        current = next;
    }
}

FAT::~FAT()
{
    free(data);
}