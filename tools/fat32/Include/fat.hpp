// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

#include <vector>
#include <string>

#include <defs.hpp>
#include <part.hpp>


namespace FAT32
{
	namespace FAT
	{
		// Defs

		constexpr uint64_t FAT_CACHE_SECTORS = 8;
		constexpr uint64_t FAT_INITIAL_HANDLE_COUNT = 16;
		
		constexpr uint32_t FAT_FIRST_USABLE_CLUSTER = 2;
		constexpr uint64_t FAT_SFN_MAX_LENGTH = 12;
		constexpr uint64_t FAT_SFN_DISK_LENGTH = 11;
		constexpr uint64_t FAT_MAX_FILE_NAME_LENGTH = 260;
		constexpr uint64_t FAT_CHARS_PER_LFN_ENTRY = 13;

		constexpr uint32_t FAT_FSINFO_LEAD_SIGNATURE = 0x41615252;
		constexpr uint32_t FAT_FSINFO_SECOND_SIGNATURE = 0x61417272;
		constexpr uint32_t FAT_FSINFO_TRAILING_SIGNATURE = 0xAA550000;

		// FAT structures

		struct PACK FAT_BIOSParameterBlock
		{
			uint8_t bootJumpInst[3];
			uint8_t oemIdentifier[8];
			uint16_t bytesPerSector;
			uint8_t sectorsPerCluster;
			uint16_t reservedSectorCount;
			uint8_t fatCount;
			uint16_t rootEntryCount;
			uint16_t totalSectors;
			uint8_t mediaDescriptorType;
			uint16_t sectorsPerFat;
			uint16_t sectorsPerTrack;
			uint16_t heads;
			uint32_t hiddenSectors;
			uint32_t largeSectorCount;
		};

		struct PACK FAT12_16_ExtendedBootRecord
		{
			uint8_t driveNumber;
			uint8_t winntFlags;
			uint8_t signature;
			uint32_t volumeID;
			uint8_t volumeName[11];
			uint8_t systemIdentStr[8];
			uint8_t bootCode[448];
			uint16_t bootSignature;
		};

		struct PACK FAT32_ExtendedBootRecord
		{
			uint32_t sectorsPerFAT;
			uint16_t flags;
			uint16_t fatVersion;
			uint32_t rootDirCluster;
			uint16_t fsInfoStruct;
			uint16_t backupBootSector;
			uint8_t _Reserved[12];
			uint8_t driveNumber;
			uint8_t winntFlags;
			uint8_t signature;
			uint32_t volumeID;
			uint8_t volumeName[11];
			uint8_t systemIdentStr[8];
			uint8_t bootCode[420];
			uint16_t bootSignature;
		};

		struct PACK FAT_FSInfo
		{
			uint32_t leadSignature;
			uint8_t _Reserved[480];
			uint32_t secondSignature;
			uint32_t lastKnownFreeClusterCount;
			uint32_t nextFreeClusterHint;
			uint32_t _Reserved1[3];
			uint32_t trailingSignature;
		};

		struct FAT_BootSector
		{
			FAT_BIOSParameterBlock bpb;
			union
			{
				FAT12_16_ExtendedBootRecord ebr1216;
				FAT32_ExtendedBootRecord ebr32;
			};
		};

		enum FAT_Attribs
		{
			FAT_ATTRIB_READONLY = 0x01,
			FAT_ATTRIB_HIDDEN = 0x02,
			FAT_ATTRIB_SYSTEM = 0x04,
			FAT_ATTRIB_VOLUMEID = 0x08,
			FAT_ATTRIB_DIRECTORY = 0x10,
			FAT_ATTRIB_ARCHIVE = 0x20,
			FAT_ATTRIB_LFN = FAT_ATTRIB_READONLY | FAT_ATTRIB_HIDDEN | FAT_ATTRIB_SYSTEM | FAT_ATTRIB_VOLUMEID
		};

		struct PACK FAT_DirectoryEntry
		{
			uint8_t shortFileName[11];
			uint8_t fileAttribs;
			uint8_t _Reserved;
			uint8_t creationTimeTenths;
			uint16_t creationTime;
			uint16_t creationDate;
			uint16_t lastAccessDate;
			uint16_t firstClusterHigh;
			uint16_t lastModificationTime;
			uint16_t lastModificationDate;
			uint16_t firstClusterLow;
			uint32_t fileSizeBytes;
		};

		struct PACK FAT_LFNEntry
		{
			uint8_t order;
			char16_t chars1[5];
			uint8_t attribute;
			uint8_t longEntryType;
			uint8_t checksum;
			char16_t chars2[6];
			uint16_t _Reserved;
			char16_t chars3[2];
		};

		// Driver structures

		enum class FAT_Types
		{
			NONE = 0,
			FAT12,
			FAT16,
			FAT32,
		};

		enum FAT_FileFlags : uint64_t
		{
			// Bit 0: file or directory? 0: file, 1: directory
			FAT_FILE_FLAG_FILE = 0,
			FAT_FILE_FLAG_DIRECTORY = (1UL << 0),

			// Bit 1: Read-only? 0: false, 1: true
			FAT_FILE_FLAG_READONLY = (1UL << 1),

			// Bit 2: Hidden? 0: false, 1: true
			FAT_FILE_FLAG_HIDDEN = (1UL << 2),
			
			// Bit 3: System? 0: false, 1: true.
			FAT_FILE_FLAG_SYSTEM = (1UL << 3),

			// Bit 4: Root Directory? 0: false, 1: true
			FAT_FILE_FLAG_ROOT_DIRECTORY = (1UL << 4),
		};

		struct FAT_FileHandle
		{
			// Validation data
			uint64_t generation;
			bool open;

			// File data
			std::wstring fileName;

			uint32_t firstCluster;
			uint32_t currentCluster;
			uint32_t currentSectorInCluster;

			uint64_t position;
			uint64_t size;
			
			uint64_t allocatedSize;

			// Extra data to locate the directory entry
			uint32_t dirEntryCluster;
			uint64_t dirEntryOffsetInCluster;

			// Extra metadata
			uint64_t flags;
		};

		struct FAT_File
		{
			uint64_t handleID;
			uint64_t generation;
		};

		struct FAT_LFNDirectoryEntry
		{
			FAT_DirectoryEntry dirEntry;
			bool hasLFN;
			std::wstring lfn;
			uint32_t cluster;
			uint64_t offsetInCluster;
		};

		enum class FAT_SeekBase
		{
			SeekFromStart,
			SeekFromEnd,
			SeekFromCurrent,
		};

		struct FAT_TimeDate
		{
			uint16_t year;
			uint8_t month, day;
			uint8_t hour, minute, second;
			uint16_t millisecond;
		};

		struct FAT_DirectoryEntryInfo
		{
			std::wstring name;
			uint32_t firstCluster;
			uint64_t size;
			uint64_t flags;
			FAT_TimeDate timeDate;
		};

		enum FAT_TimestampTypes
		{
			FAT_TIMESTAMP_TYPE_NONE = 0x00,
			FAT_TIMESTAMP_TYPE_CREATED = 0x01,
			FAT_TIMESTAMP_TYPE_ACCESSED = 0x02,
			FAT_TIMESTAMP_TYPE_MODIFIED = 0x04,
			FAT_TIMESTAMP_TYPE_ALL = FAT_TIMESTAMP_TYPE_CREATED | FAT_TIMESTAMP_TYPE_ACCESSED | FAT_TIMESTAMP_TYPE_MODIFIED,
		};

		struct FAT_CreateFSDesc
		{
			PartMgr::PartMgr* partMgr;
			PartMgr::PartDesc* partDesc;
			char volumeLabel[11];
			uint8_t fatBits;
		};

		class FAT
		{
		public:
			FAT32_STATUS Initialize(PartMgr::PartMgr* partMgr, PartMgr::PartDesc* partDesc);

			std::expected<FAT_File, FAT32_STATUS> OpenFile(const std::wstring& path, bool directory = false);
			std::expected<FAT_File, FAT32_STATUS> CreateFile(const std::wstring& path, FAT_FileFlags fileFlags, bool directory = false);

			std::expected<uint64_t, FAT32_STATUS> ReadFile(FAT_File* file, uint64_t count, void* bufferOut);
			FAT32_STATUS WriteFile(FAT_File* file, uint64_t count, void* buffer);

			std::expected<FAT_DirectoryEntryInfo, FAT32_STATUS> GetNextDirectoryEntry(FAT_File* directory);

			FAT32_STATUS Seek(FAT_File* file, int64_t offset, FAT_SeekBase base);
			FAT32_STATUS ResetPos(FAT_File* file);

			std::expected<uint64_t, FAT32_STATUS> GetFileSize(FAT_File* file);
			FAT32_STATUS GetVolumeLabel(std::string& strOut);
			uint64_t GetFreeByteCount();

			~FAT();
		private:
			std::expected<FAT_FileHandle, FAT32_STATUS> OpenFileHandle(std::wstring_view path, bool directory);
			std::expected<FAT_FileHandle, FAT32_STATUS> OpenFileHandleInDir(FAT_FileHandle dirHandle, const std::wstring_view name, bool directory);

			std::expected<FAT_FileHandle, FAT32_STATUS> CreateFileHandle(std::wstring_view path, FAT_FileFlags fileFlags, bool directory);
			std::expected<FAT_FileHandle, FAT32_STATUS> CreateFileHandleInDir(FAT_FileHandle dirHandle, const std::wstring_view name, FAT_FileFlags fileFlags, bool directory, FAT_DirectoryEntry* parentEntry);

			FAT32_STATUS ExpandFile(FAT_FileHandle* fileHandle, size_t newEnd);

			std::expected<FAT_LFNDirectoryEntry, FAT32_STATUS> ReadEntry(FAT_FileHandle* dirHandle);
			std::expected<std::pair<uint32_t, uint64_t>, FAT32_STATUS> FindFreeEntryGap(FAT_FileHandle* dirHandle, size_t totalEntries);
			std::expected<FAT_LFNDirectoryEntry, FAT32_STATUS> OpenDirectoryEntry(FAT_FileHandle dirHandle, const std::wstring_view name, bool directory);

			uint64_t AllocateHandle();
			uint64_t RegisterHandle(FAT_FileHandle handle);

			uint64_t ClusterToLba(uint32_t cluster);
			std::expected<uint32_t, FAT32_STATUS> NextCluster(uint32_t previous);
			std::expected<uint32_t, FAT32_STATUS> AllocateCluster(uint32_t previousLast);
			FAT32_STATUS ZeroCluster(uint32_t cluster);
			std::expected<uint32_t, FAT32_STATUS> GetClusterChainLength(uint32_t startCluster);
			std::expected<uint32_t, FAT32_STATUS> GetLastCluster(uint32_t chainCluster);

			std::expected<uint32_t, FAT32_STATUS> ReadFAT(uint32_t cluster);
			FAT32_STATUS WriteFAT(uint32_t cluster, uint32_t value);

			bool ConvertNameToSFN(const std::wstring_view name, bool directory, char* bufferOut);
			bool ConvertNameToSFN(const std::string_view name, bool directory, char* bufferOut);
			void ConvertSFNToName(char* sfnBuffer, bool directory, std::wstring& nameOut);

			void GenerateLFNAliasSFN(const std::wstring_view name, bool directory, char* bufferOut);
			uint8_t CalculateSFNChecksum(char* sfn);

			FAT32_STATUS NextSector(FAT_FileHandle* handle);

			void AppendLFNChars(const char16_t* chars, std::wstring& strOut, size_t count);
			std::expected<std::vector<FAT_LFNEntry>, FAT32_STATUS> ConstructLFNEntries(const std::wstring_view name, uint8_t sfnChecksum);

			FAT_TimeDate GetTimeDateFromDirEntry(FAT_DirectoryEntry* entry);
			FAT_TimeDate GetCurrentTimeDate();
			void SetDirectoryEntryTimeDate(FAT_DirectoryEntry* entry, FAT_TimeDate* td, FAT_TimestampTypes update);

			PartMgr::PartMgr* partMgr;
			PartMgr::PartDesc* partDesc;

			void* fatCache;
			uint64_t fatCacheSector;
			bool cacheDirty;
			
			FAT_BootSector bootSector;
			FAT_FSInfo fsInfo;
			bool fsInfoPresent;
			
			FAT_Types fatType;
			uint32_t totalSectors;
			uint32_t fatSectors;
			uint32_t rootDirSectors;
			uint32_t dataRegionStartSector;
			uint32_t fatStartSector;
			uint32_t totalDataSectors;
			uint32_t totalClusters;

			uint64_t clusterSize;

			uint32_t EOCTreshold;

			FAT_File rootDirectory;
			FAT_DirectoryEntry rootDirEntry;
			std::vector<FAT_FileHandle> handles;
		};
	}
}