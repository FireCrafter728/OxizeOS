#pragma once
#include <disk.hpp>
#include <gpt.hpp>

#define FAT_CACHE_SIZE 10
#define FAT_MAX_LFN_ENTRIES 20
#define FAT_MAX_FILE_NAME_SIZE 256
#define FAT_MAX_PATH_SIZE 260

namespace FAT32
{
    namespace FAT
    {
        struct __attribute__((packed)) BPB
        {
            m_uint8_t bootJumpInstruction[3];
            m_uint8_t oem[8];
            m_uint16_t BytesPerSector;
            m_uint8_t SectorsPerCluster;
            m_uint16_t ReservedSectors;
            m_uint8_t FATCount;
            m_uint16_t rootDirEntries;
            m_uint16_t totalSectors16;
            m_uint8_t mediaDescType;
            m_uint16_t _Reserved;
            m_uint16_t SectorsPerTrack;
            m_uint16_t HeadCount;
            m_uint32_t HiddenSectors;
            m_uint32_t totalSectors32;
        };

        struct __attribute__((packed)) EBR
        {
            m_uint32_t SectorsPerFAT;
            m_uint16_t Flags;
            m_uint16_t VersionNumber;
            m_uint32_t RootDirCluster;
            m_uint16_t FSInfoSector;
            m_uint16_t backupBootSector;
            m_uint8_t _Reserved[12];
            m_uint8_t DriveNumber;
            m_uint8_t _Reserved0;
            m_uint8_t Signature;
            m_uint32_t VolumeID;
            m_uint8_t VolumeLabel[11];
            m_uint8_t SystemIdentifier[8];
        };

        struct __attribute__((packed)) BootSector
        {
            BPB bpb;
            EBR ebr;
            m_uint8_t bootCode[420];
            m_uint16_t bootSig;
        };

        enum FileAttribs : m_uint8_t
        {
            READONLY = 0x01,
            HIDDEN = 0x02,
            SYSTEM = 0x04,
            VOLUMEID = 0x08,
            DIRECTORY = 0x10,
            ARCHIVE = 0x20,
            LFN = READONLY | HIDDEN | SYSTEM | VOLUMEID
        };

        struct __attribute__((packed)) DirectoryEntry
        {
            m_uint8_t Name[11];
            m_uint8_t Attribs;
            m_uint8_t _Reserved;
            m_uint8_t CreatedTimeHundreads;
            m_uint16_t CreationTime;
            m_uint16_t CreationDate;
            m_uint16_t LastAccessedDate;
            m_uint16_t FirstClusterHigh;
            m_uint16_t LastModifiedTime;
            m_uint16_t LastModifiedDate;
            m_uint16_t FirstClusterLow;
            m_uint32_t FileSize;
        };

        struct __attribute__((packed)) LFNEntry
        {
            m_uint8_t Order;
            wchar Entry1[5];
            m_uint8_t Attrib;
            m_uint8_t EntryType;
            m_uint8_t Checksum;
            wchar Entry2[6];
            m_uint16_t _Reserved;
            wchar Entry3[2];
        };

        struct LFNDirectoryEntry
        {
            DirectoryEntry SFNEntry;
            char LFN[FAT_MAX_FILE_NAME_SIZE];
            bool IsLFN = false;
            int LFNEntryCount;
        };

        struct File
        {
            m_uint32_t Size, Position;
            m_uint32_t FirstCluster, CurrentCluster;
            m_uint8_t CurrentSectorInCluster;
            LFNDirectoryEntry fileEntry;
            bool IsDirectory;
        };

        struct __attribute__((packed)) FSInfo {
            m_uint32_t LeadSignature;
            m_uint8_t _Reserved[480];
            m_uint32_t StructSignature;
            m_uint32_t FreeCount;
            m_uint32_t NextFree;
            m_uint8_t _Reserved0[12];
            m_uint16_t TrailSignature;
        };

        struct Data
        {
            union
            {
                BootSector bootSector;
                m_uint8_t bootSectorBytes[SECTOR_SIZE];
            } BS;
            File rootDirectory;
            m_uint8_t FatCache[FAT_CACHE_SIZE * SECTOR_SIZE];
            m_uint32_t FatCacheSector;
        };
        struct FatData
        {
            m_uint32_t TotalSectors;
            m_uint32_t TotalClusters;
            m_uint32_t DataSectionLba;
            m_uint32_t FatStartLba;
        };
        struct MKFSDesc
        {
            m_uint16_t BytesPerSector = 512;
            m_uint16_t SectorsPerCluster = 0; // if value is 0, determine it manually
            m_uint8_t mediaDescType = 0xF8;
            m_uint16_t SectorsPerTrack = 63;
            m_uint16_t HeadCount = 255;
            m_uint8_t DriveNumber = 0x80;
            m_uint32_t VolumeID = 0;
            m_uint8_t VolumeLabel[11] = {'F', 'A', 'T', '3', '2', ' ', ' ', ' ', ' ', ' ', ' '};
        };
        class FAT
        {
        public:
            bool Initialize(GPT::GPT *gpt);
            void InitializeMin(GPT::GPT *gpt);
            bool mkfs(MKFSDesc* desc);
            bool OpenFile(LPCSTR path, File *file);
            m_uint32_t ReadFile(File *file, void *buffer, m_uint32_t size);
            bool CreateEntry(LPCSTR path, bool isDirectory, File *fileOut);
            bool DeleteEntry(LPCSTR path, bool isDirectory);
            bool DeleteDir(LPCSTR path);
            bool RenameEntry(LPCSTR path, LPCSTR newName, bool isDirectory, File *oldFile);
            bool CopyEntry(LPCSTR path, LPCSTR newPath, bool isDirectory, File *newFile);
            bool MoveEntry(LPCSTR path, LPCSTR newPath, bool isDirectory, File *newFile);

            bool SetFileData(LPCSTR filePath, void *data, m_uint32_t count);

            void Seek(File *file, m_uint32_t pos);
            void ResetPos(File *file);

            bool ReadEntry(File *dir, LFNDirectoryEntry *entryOut);

            void GetTimeFormatted(m_uint16_t Time, LPSTR strOut);
            void GetDateFormatted(m_uint16_t Date, LPSTR strOut);
            m_uint32_t GetBytesFree();

            ~FAT();

        private:
            GPT::GPT *gpt = nullptr;
            FatData fatData = {};
            Data *data = nullptr;
            BPB *bpb = nullptr;
            EBR *ebr = nullptr;

            m_uint32_t ClusterToLba(m_uint32_t cluster);
            m_uint32_t LbaToCluster(m_uint32_t lba);
            bool NextCluster(m_uint32_t &cluster);
            m_uint32_t AllocateCluster();
            void ZeroCluster(m_uint32_t cluster);

            bool ReadBootSector();
            bool ReadFAT(m_uint32_t lbaIndex);
            m_uint32_t ReadFATEntry(m_uint32_t cluster);
            bool WriteFATEntry(m_uint32_t cluster, m_uint32_t value);

            bool FindFile(LPCSTR path, LFNDirectoryEntry *entryOut);
            bool FindFileInDir(File *dir, LPCSTR name, LFNDirectoryEntry *entryOut);
            bool OpenFileInDir(File *dir, LPCSTR name, File *file);

            bool AssembleLFN(LFNEntry *entries, int count, char *outName);
            void GetShortName(const char *name, char shortName[11]);
            bool IsValidShortName(LPCSTR name);
            bool IsValidEntryName(LPCSTR name);

            m_uint8_t CalculateShortNameChecksum(m_uint8_t shortName[11]);
            void FreeClusterChain(m_uint32_t startCluster);

            bool CreateDirectoryEntry(File *dir, LPCSTR name, LFNDirectoryEntry *entry);
        };
    }
}