// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <cstddef>
#include <expected>

#include <defs.hpp>
#include <disk.hpp>

namespace FAT32
{
	namespace PartMgr
	{
		constexpr size_t MBR_TABLE_SECTORS = 1;
		constexpr size_t MBR_TABLE_SIZE = MBR_TABLE_SECTORS * SECTOR_SIZE;

		struct PACK MBR_PartitionEntry
		{
			uint8_t driveAttribs;
			CHS partitionStartCHS;
			uint8_t partitionType;
			CHS partitionLastSectorCHS;
			uint32_t partitionStartLBA;
			uint32_t partitionSectors;
		};

		struct PACK MBR_Header
		{
			uint8_t bootcode[440];
			uint32_t diskID;
			uint16_t reserved;
			MBR_PartitionEntry partEntries[4];
			uint16_t bootSig;
		};

		// Back in the DOS times when more partitions were needed, extended & logical partitions were designed,
		// However an extended partition occupied an entire sector, however it only described a single logical partition
		// And pointed to the next extended partition. That can result in the HDD having to read from locations
		// that are physically far from eachother, which on an HDD is very slow access.
		//
		// A design that would've been far more efficient is where an extended partition held
		// 32 total MBR-style partition entries, with an entry being reserved to describe
		// the next extended partition, and the rest to describe 31 logical partitions,
		// and to find out the extended partition and offset in extended partition,
		// modulo or division can be used to get the exact offsets

		struct PACK MBR_ExtendedPartition
		{
			uint8_t _Unused[446];
			MBR_PartitionEntry entries[4];
			uint16_t _Unused2;
		};

		constexpr uint8_t EXTENDED_PART_CHS_TYPE = 0x05;
		constexpr uint8_t EXTENDED_PART_LBA_TYPE = 0x0F;
		constexpr uint8_t EXTENDED_PART_LINUX_TYPE = 0x85;

		struct PACK GPT_PartitionEntry
		{
			FAT32_GUID PartType;
			FAT32_GUID UniqueIdentifier;
			uint64_t StartLBA, EndLBA;
			uint64_t Flags;
			uint16_t PartitionNameUnicode[36];
		};

		struct PACK GPT_Header
		{
			uint64_t Signature;
			uint32_t Revision;
			uint32_t HdrSize;
			crc32_t HdrCRC32;
			uint32_t _Reserved;
			uint64_t CurrentLBA;
			uint64_t BackupLBA;
			uint64_t UsableLBAStart;
			uint64_t UsableLBAEnd;
			FAT32_GUID DiskGUID;
			uint64_t PartEntryStartLBA;
			uint32_t PartEntryCount;
			uint32_t PartEntrySize;
			crc32_t PartEntryCRC32;
			uint8_t Padding[420];
		};

		struct PACK GPT_Desc
		{
			MBR_Header ProtectiveMBR;
			GPT_Header header;
		};

		enum PartTableType : uint8_t
		{
			PART_TYPE_UNKNOWN = 0,
			PART_TYPE_MBR,
			PART_TYPE_GPT,
		};

		struct PartDesc
		{
			FAT32_GUID diskGUID; // GPT only
			uint32_t diskID; // MBR only
			uint8_t partIndexInDisk;
			uint64_t partStartLBA, partEndLBA;
			bool readonly;
		};

		enum VerificationFlags : uint64_t
		{
			VERIF_INVALID_HDR_SIZE = (1ULL << 0),
			VERIF_INVALID_CURR_LBA = (1ULL << 1),
			VERIF_INVALID_BACKUP_LBA = (1ULL << 2),
			VERIF_INVALID_USABLE_LBA_START = (1ULL << 3),
			VERIF_INVALID_USABLE_LBA_END = (1ULL << 4),
			VERIF_INVALID_PART_ENTRY_SIZE = (1ULL << 5),
			VERIF_INVALID_PART_ENTRY_COUNT = (1ULL << 6),
			VERIF_INVALID_PART_ENTRY_START = (1ULL << 7),
			VERIF_INVALID_PART_ENTRY_CRC32 = (1ULL << 8),
			VERIF_INVALID_GPT_HEADER_CRC32 = (1ULL << 9),
		};

		static constexpr const char GPT_Signature[8] = {'E','F','I',' ','P','A','R','T'};

		constexpr size_t GPTHeaderSize = offsetof(GPT_Header, Padding);

		class PartMgr
		{
		public:
			FAT32_STATUS Initialize(DISK* disk);

			std::expected<PartDesc*, FAT32_STATUS> OpenPartitionByIndex(uint32_t partIndex);
			std::expected<PartDesc*, FAT32_STATUS> OpenPartitionByGUID(FAT32_GUID partGUID);
			FAT32_STATUS ClosePartition(PartDesc* desc);

			FAT32_STATUS ReadSectors(PartDesc* desc, uint64_t lba, size_t count, void* bufferOut);
			FAT32_STATUS WriteSectors(PartDesc* desc, uint64_t lba, size_t count, void* buffer);

			inline uint64_t GetPartitionStartLBA(PartDesc* desc) { return desc->partStartLBA; }
			inline uint64_t GetPartitionTotalSectors(PartDesc* desc) { return desc->partEndLBA - desc->partStartLBA; }

			~PartMgr();
		private:
			DISK* disk;
			PartTableType tableType;
			FAT32_GUID diskGUID; // GPT only
			uint32_t diskID; // MBR only
			void* table;
			void* backupTable;
			uint64_t diskSectorCount;

			FAT32_STATUS InitializeGPT();
			FAT32_STATUS InitializeMBR();
		
			std::expected<uint64_t, FAT32_STATUS> VerifyGPT(GPT_Header* hdr, bool backup, bool quiet = false);
			bool VerifyGPTConsistency(GPT_Header* faultyHeader, GPT_Header* validHeader);
			FAT32_STATUS RecoverGPT(GPT_Header* primaryHeader, GPT_Header* backupHeader, bool recoverBackup, uint64_t verificationFlags);

			std::expected<GPT_PartitionEntry, FAT32_STATUS> OpenGPTPartitionEntry(uint32_t partitionIndex);

			FAT32_STATUS OpenGPTPartition(GPT_PartitionEntry* entry, PartDesc* desc, uint32_t partitionIndex);
			FAT32_STATUS OpenMBRPartition(MBR_PartitionEntry* entry, PartDesc* desc, uint32_t partitionIndex);

			inline bool IsExtendedPartition(MBR_PartitionEntry* entry) { return (entry->partitionType == EXTENDED_PART_CHS_TYPE || entry->partitionType == EXTENDED_PART_LBA_TYPE || entry->partitionType == EXTENDED_PART_LINUX_TYPE); }
		};
	}
}