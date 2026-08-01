// SPDX-License-Identifier: GPL-3.0-or-later

#include <part.hpp>
#include <utils.hpp>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <algorithm>

using namespace FAT32::PartMgr;

FAT32_STATUS PartMgr::Initialize(DISK* disk)
{
	if(!disk)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Invalid Initialize input parameters\n");
		return FAT32_INVALID_PARAMETER;
	}

	this->disk = disk;
	this->diskSectorCount = disk->GetDiskSectorCount();

	if(this->diskSectorCount == 0)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to get disk sector count\n");
		return FAT32_DISK_ERROR;
	}

	this->backupTable = nullptr;
	this->table = malloc(SECTOR_SIZE);
	if(!this->table)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to allocate memory for the partition tables\n");
		return FAT32_MEMORY_ALLOCATION_FAILED;
	}

	// Parse the disk header

	// To detect whether the disk is GPT or MBR, the best way is by checking if the 2nd sector of the disk contains the GPT signature "EFI PART". No -> disk is probably MBR, yes -> disk is probably GPT

	FAT32_STATUS status = disk->ReadSectors(1, 1, this->table);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to read from DISK, lba: 1, count: 1\n");
		return status;
	}

	if(memcmp(this->table, GPT_Signature, 8) == 0)
	{
		// GPT Signature matches, partition type is GPT
		this->tableType = PART_TYPE_GPT;

		return InitializeGPT();
	}

	// Disk is probably MBR

	// Read MBR into the disk
	status = disk->ReadSectors(0, 1, this->table);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to read from DISK, lba: 0, count: 1\n");
		return status;
	}

	this->tableType = PART_TYPE_MBR;

	return InitializeMBR();
}

FAT32_STATUS PartMgr::InitializeGPT()
{
	FAT32_STATUS status;
	GPT_Header* primaryHeader = reinterpret_cast<GPT_Header*>(this->table);
	this->diskGUID = primaryHeader->DiskGUID;

	// Verify the primary GPT Header
	auto verifRes = VerifyGPT(primaryHeader, false);
	if(!verifRes) return verifRes.error();
	uint64_t primaryFlags = verifRes.value();
	bool isPrimaryGPTValid = primaryFlags == 0;

	// Check if the backup GPT is accessible
	if(FLAG_GET_BOOLEAN(primaryFlags, VERIF_INVALID_BACKUP_LBA))
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot recover primary GPT Header as backup GPT Header is inaccessible\n");
		return FAT32_INVALID_PARTITION_TABLE;
	}

	// Allocate memory for the backup GPT header and read it from the disk into the allocated memory
	this->backupTable = malloc(SECTOR_SIZE);
	if(!this->backupTable)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to allocate memory to read the backup GPT Header to\n");
		return FAT32_MEMORY_ALLOCATION_FAILED;
	}

	status = disk->ReadSectors(primaryHeader->BackupLBA, 1, this->backupTable);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to read backup GPT from DISK, lba: %lu, count: 1, disk GUID: " GUID_PRINT_FORMAT "\n", primaryHeader->BackupLBA, DECODE_GUID_TO_ARG(this->diskGUID));
		return status;
	}

	GPT_Header* backupHeader = reinterpret_cast<GPT_Header*>(this->backupTable);

	// Check if the backup GPT Header signature matches
	if(memcmp(backupHeader, GPT_Signature, 8) != 0)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Couldn't find the backup GPT Header and partition table\n");
		if(!isPrimaryGPTValid)
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Couldn't recover the primary GPT because the backup GPT is not present\n");
			return FAT32_CORRUPTED_BACKUP_PARTITION_TABLE;
		}
		else return FAT32_SUCCESS;
	}

	// Verify the backup GPT
	verifRes = VerifyGPT(backupHeader, true);
	if(!verifRes) return verifRes.error();
	uint64_t backupFlags = verifRes.value();
	bool isBackupGPTValid = backupFlags == 0;

	// If both headers seem valid after standalone checks, perform consistency validations
	if(isPrimaryGPTValid && isBackupGPTValid)
	{
		if(!VerifyGPTConsistency(primaryHeader, backupHeader)) return FAT32_INCONSISTENT_PARTITION_TABLES;
		return FAT32_SUCCESS;
	}

	// Check if both GPTs are invalid
	if(!isPrimaryGPTValid && !isBackupGPTValid)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Couldn't attempt recovery on the primary GPT as the backup GPT is also invalid\n");
		return FAT32_PARTITION_TABLE_RECOVERY_FAILED;
	}

	// Attempt recovery on the invalid GPT
	if(!isPrimaryGPTValid) return RecoverGPT(primaryHeader, backupHeader, false, primaryFlags);
	if(!isBackupGPTValid) return RecoverGPT(backupHeader, primaryHeader, true, backupFlags);

	return FAT32_SUCCESS;
}

FAT32_STATUS PartMgr::InitializeMBR()
{
	// Initialize() should've already read the MBR to the table ptr

	MBR_Header* mbr = reinterpret_cast<MBR_Header*>(this->table);

	// Check for the boot signature(0xAA55, little endian)
	if(mbr->bootSig != 0xAA55)
	{
		printf("[FAT32] [PARTMGR] [ERROR]: Cannot initialize the partition manager on a disk that's neither GPT, neither MBR\n");
		return FAT32_UNKNOWN_OR_NO_PARTITION_TABLE;
	}

	this->diskGUID = emptyGUID;
	this->diskID = mbr->diskID;

	return FAT32_SUCCESS;
}

std::expected<uint64_t, FAT32_STATUS> PartMgr::VerifyGPT(GPT_Header* hdr, bool backup, bool quiet)
{
	uint64_t flags = 0;
	FAT32_STATUS status;

	FILE* errorstream = stderr;
	if(quiet) errorstream = fopen("/dev/null", "w");
	if(!errorstream) errorstream = stderr;

	// Check if revision is 1.0
	if(((hdr->Revision >> 16) & 0xFFFF) != 1 || (hdr->Revision & 0xFFFF) != 0)
		fprintf(errorstream, "[FAT32] [PARTMGR] [WARN]: Unknown %s GPT Revision %u.%u, expected revision 1.0, disk GUID: " GUID_PRINT_FORMAT "\n", backup ? "backup" : "primary", (hdr->Revision >> 16) & 0xFFFF, hdr->Revision & 0xFFFF, DECODE_GUID_TO_ARG(this->diskGUID));
	
	// Check if the header size is between 92 & 512

	if(hdr->HdrSize < GPTHeaderSize || hdr->HdrSize > SECTOR_SIZE)
	{
		fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: Invalid %s GPT Header HdrSize field, disk GUID: " GUID_PRINT_FORMAT "\n", backup ? "backup" : "primary", DECODE_GUID_TO_ARG(this->diskGUID));
		FLAG_SET(flags, VERIF_INVALID_HDR_SIZE);
	}

	if(!FLAG_GET_BOOLEAN(flags, VERIF_INVALID_HDR_SIZE) && hdr->HdrSize != GPTHeaderSize) fprintf(errorstream, "[FAT32] [PARTMGR] [WARN]: Expected %s GPT Header HdrSize field value to be 92 bytes, got %u, disk GUID: " GUID_PRINT_FORMAT "\n", backup ? "backup" : "primary", hdr->HdrSize, DECODE_GUID_TO_ARG(this->diskGUID));

	// Check if the reserved field is NULL

	if(hdr->_Reserved != 0) fprintf(errorstream, "[FAT32] [PARTMGR] [WARN]: %s GPT Header reserved field is not set to NULL, disk GUID: " GUID_PRINT_FORMAT "\n", backup ? "backup" : "primary", DECODE_GUID_TO_ARG(this->diskGUID));

	if(!backup)
	{
		// Check if the primary GPT header CurrentLBA field is correct(set to LBA 1)
		if(hdr->CurrentLBA != 1)
		{
			fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: Invalid primary GPT Header CurrentLBA field, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
			FLAG_SET(flags, VERIF_INVALID_CURR_LBA);
		}

		// Check if backup LBA isn't above the disk sector count
		if(hdr->BackupLBA >= this->diskSectorCount)
		{
			fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: primary GPT Header backup LBA is above the disk sector count, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
			FLAG_SET(flags, VERIF_INVALID_BACKUP_LBA);
		}
	}
	else
	{
		// Check if the backup GPT header CurrentLBA field isn't above the disk sector count
		if(hdr->CurrentLBA >= this->diskSectorCount)
		{
			fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: backup GPT Header current LBA is above the disk sector count, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
			FLAG_SET(flags, VERIF_INVALID_CURR_LBA);
		}

		if(hdr->CurrentLBA == 0)
		{
			fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: backup GPT Header current LBA is NULL, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
			FLAG_SET(flags, VERIF_INVALID_CURR_LBA);
		}

		// Check if backup LBA is correct(set to LBA 1)
		if(hdr->BackupLBA != 1)
		{
			fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: Invalid backup GPT Header backup LBA field, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
			FLAG_SET(flags, VERIF_INVALID_BACKUP_LBA);
		}
	}

	// Check if backup LBA field isn't the same as current LBA field
	if(hdr->BackupLBA == hdr->CurrentLBA)
	{
		fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: %s GPT Header backup LBA is the same as the current LBA, disk GUID: " GUID_PRINT_FORMAT "\n", backup ? "backup" : "primary", DECODE_GUID_TO_ARG(this->diskGUID));
		// backupLbaRecoveryNeeded = true;
		FLAG_SET(flags, VERIF_INVALID_BACKUP_LBA);
		FLAG_SET(flags, VERIF_INVALID_CURR_LBA);
	}

	// Check if Usable LBA Start isn't above Usable LBA End
	if(hdr->UsableLBAStart > hdr->UsableLBAEnd)
	{
		fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: %s GPT Header Usable LBA Start is above the Usable LBA End, disk GUID: " GUID_PRINT_FORMAT "\n", backup ? "backup" : "primary", DECODE_GUID_TO_ARG(this->diskGUID));
		FLAG_SET(flags, VERIF_INVALID_USABLE_LBA_START);
	}

	// Check if Parition Entry Size is above 128 bytes and is 8-byte aligned
	if(hdr->PartEntrySize < sizeof(GPT_PartitionEntry))
	{
		fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: %s GPT Header Partition Entry Size is below the minimum size of %lu, disk GUID: " GUID_PRINT_FORMAT "\n", backup ? "backup" : "primary", sizeof(GPT_PartitionEntry), DECODE_GUID_TO_ARG(this->diskGUID));
		FLAG_SET(flags, VERIF_INVALID_PART_ENTRY_SIZE);
	}

	// Print warning if the size is bigger than expected
	if(!FLAG_GET_BOOLEAN(flags, VERIF_INVALID_PART_ENTRY_SIZE) && hdr->PartEntrySize != sizeof(GPT_PartitionEntry))
	{
		fprintf(errorstream, "[FAT32] [PARTMGR] [WARN]: Expected %s GPT Header Partition Entry Size field value to be %lu bytes, got %u, disk GUID: " GUID_PRINT_FORMAT "\n", backup ? "backup" : "primary", sizeof(GPT_PartitionEntry), hdr->PartEntrySize, DECODE_GUID_TO_ARG(this->diskGUID));
	}

	if(!FLAG_GET_BOOLEAN(flags, VERIF_INVALID_PART_ENTRY_SIZE) && hdr->PartEntrySize % 8 != 0)
	{
		fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: %s GPT Header Partition Entry Size is not 8-byte aligned, disk GUID: " GUID_PRINT_FORMAT "\n", backup ? "backup" : "primary", DECODE_GUID_TO_ARG(this->diskGUID));
		FLAG_SET(flags, VERIF_INVALID_PART_ENTRY_SIZE);
	}

	// Check if partition entry count is not NULL
	if(hdr->PartEntryCount == 0)
	{
		fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: %s GPT Header Partition Entry Count is NULL, disk GUID: " GUID_PRINT_FORMAT "\n", backup ? "backup" : "primary", DECODE_GUID_TO_ARG(this->diskGUID));
		FLAG_SET(flags, VERIF_INVALID_PART_ENTRY_COUNT);
	}

	uint64_t partitionTableSize = static_cast<uint64_t>(hdr->PartEntryCount) * hdr->PartEntrySize;

	// Check if partition entry start LBA is not above or equal to disk sector count
	if(hdr->PartEntryStartLBA >= this->diskSectorCount)
	{
		fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: %s GPT Header partition entry start LBA is above or equal to disk sector count, disk GUID: " GUID_PRINT_FORMAT "\n", backup ? "backup" : "primary", DECODE_GUID_TO_ARG(this->diskGUID));
		FLAG_SET(flags, VERIF_INVALID_PART_ENTRY_START);
	}

	// Check if partition entry array end is not above the disk sector count
	if(hdr->PartEntryStartLBA + SECTOR_COUNT(partitionTableSize) > this->diskSectorCount)
	{
		fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: %s GPT Header partition entry array end sector is above the disk sector count, disk GUID: " GUID_PRINT_FORMAT "\n", backup ? "backup" : "primary", DECODE_GUID_TO_ARG(this->diskGUID));
		FLAG_SET(flags, VERIF_INVALID_PART_ENTRY_START);
	}

	if(!backup)
	{
		// Check if Usable LBA Start doesn't cover the primary GPT Header and partition entries
		if(hdr->UsableLBAStart < hdr->PartEntryStartLBA + SECTOR_COUNT(partitionTableSize))
		{
			fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: primary GPT Header Usable LBA Start is below the GPT Header end, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
			FLAG_SET(flags, VERIF_INVALID_USABLE_LBA_START);
		}

		// Check if the Usable LBA End isn't above the disk sector count
		if(hdr->UsableLBAEnd >= this->diskSectorCount)
		{
			fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: primary GPT Header Usable LBA End is above the disk last LBA, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
			FLAG_SET(flags, VERIF_INVALID_USABLE_LBA_END);
		}
	}
	else
	{
		if(hdr->UsableLBAEnd >= hdr->PartEntryStartLBA)
		{
			fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: backup GPT Header Usable LBA End is above the backup partition tables, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
			FLAG_SET(flags, VERIF_INVALID_USABLE_LBA_END);
		}
	}

	// If no errors were found, validate the GPT Header CRC32
	if(flags == 0)
	{
		// To validate the GPT Header CRC32, store the CRC32 somewhere, set it to 0 in the header, compute the entire header CRC32, using the size inside the GPT Header, and compare the stored value and computed value
		GPT_Header temp = *hdr;
		crc32_t headerCRC32 = hdr->HdrCRC32;
		temp.HdrCRC32 = 0;
		crc32_t actualHeaderCRC32 = ComputeCRC32(&temp, hdr->HdrSize);
		if(headerCRC32 != actualHeaderCRC32)
		{
			fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: %s GPT Header CRC32 is invalid, stored: 0x%X, computed: 0x%X, disk GUID: " GUID_PRINT_FORMAT "\n", backup ? "backup" : "primary", headerCRC32, actualHeaderCRC32, DECODE_GUID_TO_ARG(this->diskGUID));
			FLAG_SET(flags, VERIF_INVALID_GPT_HEADER_CRC32);
		}
	}

	// If no errors were found so far, validate the partition entry array CRC32
	if(flags == 0)
	{
		// Allocate the partitionTable ptr and read to it from the DISK
		void* partitionTable = nullptr;
		size_t partEntryArraySize = partitionTableSize;
		partitionTable = malloc(SECTOR_ALIGN_UP(partEntryArraySize));
		if(!partitionTable)
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to allocate memory for the %s partition entry array\n", backup ? "backup" : "primary");
			return std::unexpected(FAT32_MEMORY_ALLOCATION_FAILED);
		}
		
		status = disk->ReadSectors(hdr->PartEntryStartLBA, SECTOR_COUNT(partEntryArraySize), partitionTable);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to read %s GPT partition table from DISK, lba: %lu, count: %lu, disk GUID: " GUID_PRINT_FORMAT "\n", backup ? "backup" : "primary", hdr->PartEntryStartLBA, SECTOR_COUNT(partEntryArraySize), DECODE_GUID_TO_ARG(this->diskGUID));
			free(partitionTable);
			return std::unexpected(status);
		}

		// To validate the partition entry array CRC32, compute the CRC32 across the entire partition entry array, which's size is partEntryCount * partEntrySize, then compare the CRC32s
		crc32_t actualPartEntryArrayCRC32 = ComputeCRC32(partitionTable, partEntryArraySize);
		free(partitionTable);
		if(hdr->PartEntryCRC32 != actualPartEntryArrayCRC32)
		{
			fprintf(errorstream, "[FAT32] [PARTMGR] [ERROR]: Invalid %s GPT partition array CRC32, stored: 0x%X, computed: 0x%X, disk GUID: " GUID_PRINT_FORMAT "\n", backup ? "backup" : "primary", hdr->PartEntryCRC32, actualPartEntryArrayCRC32, DECODE_GUID_TO_ARG(this->diskGUID));
			FLAG_SET(flags, VERIF_INVALID_PART_ENTRY_CRC32);
		}
	}

	return flags;
}

bool PartMgr::VerifyGPTConsistency(GPT_Header* primaryHeader, GPT_Header* backupHeader)
{
	// Should be the same: Signature, Revision(not needed to check), HdrSize, _Reserved(not needed to check), UsableLBAStart, UsableLBAEnd, DiskGUID, PartEntryCount, PartEntrySize, PartEntryCRC32

	if(primaryHeader->Signature != backupHeader->Signature)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Primary and backup GPT Header signatures don't match, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
		return false;
	}

	if(primaryHeader->HdrSize != backupHeader->HdrSize)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Primary and backup GPT Header HdrSize values don't match, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
		return false;
	}

	if(primaryHeader->UsableLBAStart != backupHeader->UsableLBAStart)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Primary and backup GPT Header Usable LBA Start values don't match, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
		return false;
	}
	
	if(primaryHeader->UsableLBAEnd != backupHeader->UsableLBAEnd)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Primary and backup GPT Header Usable LBA End values don't match, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
		return false;
	}

	if(primaryHeader->DiskGUID != backupHeader->DiskGUID)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Primary and backup GPT Disk GUIDs don't match, primary disk GUID: " GUID_PRINT_FORMAT ", secondary disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID), DECODE_GUID_TO_ARG(this->diskGUID));
		return false;
	}

	if(primaryHeader->PartEntryCount != backupHeader->PartEntryCount)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Primary and backup GPT Partition Entry Count values don't match, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
		return false;
	}

	if(primaryHeader->PartEntrySize != backupHeader->PartEntrySize)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Primary and backup GPT Partition Entry Size values don't match, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
		return false;
	}

	if(primaryHeader->PartEntryCRC32 != backupHeader->PartEntryCRC32)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Primary and backup GPT Partition Entry CRC32s don't match, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
		return false;
	}

	// Shouldn't be the same: HdrCRC32, CurrentLBA, BackupLBA and PartEntryStartLBA

	if(primaryHeader->HdrCRC32 == backupHeader->HdrCRC32)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Primary and backup GPT Header CRC32s are the same, when they shouldn't be, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
		return false;
	}

	if(primaryHeader->CurrentLBA == backupHeader->CurrentLBA)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Primary and backup GPT Header CurrentLBA values are the same, when they shouldn't be, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
		return false;
	}

	if(primaryHeader->BackupLBA == backupHeader->BackupLBA)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Primary and backup GPT Header BackupLBA values are the same, when they shouldn't be, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
		return false;
	}

	if(primaryHeader->PartEntryStartLBA == backupHeader->PartEntryStartLBA)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Primary and backup GPT Header Partition Entry array start LBA values are the same, when they shouldn't be, disk GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
		return false;
	}

	return true;
}

FAT32_STATUS PartMgr::RecoverGPT(GPT_Header* faultyHeader, GPT_Header* validHeader, bool recoverBackup, uint64_t verificationFlags)
{
	FAT32_STATUS status;

	char choice;
	while(true)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [INFO]: Do you want to attempt %s GPT recovery? Y/n: ", recoverBackup ? "backup" : "primary");

		char buffer[16];
		fgets(buffer, sizeof(buffer), stdin);
		choice = toupper(buffer[0]);
		if(choice == 'N')
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Use GPT fdisk(gdisk) or another disk recovery tool to recover the GUID Partition Table\n");
			return FAT32_PARTITION_TABLE_RECOVERY_DENIED_BY_USER;
		}
		if(choice == 'Y') break;
		else
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Unknown choice %c, please try again.\n", choice);
			continue;
		}
	}

	fprintf(stderr, "[FAT32] [PARTMGR] [INFO]: Attempting recovery on %s GPT\n", recoverBackup ? "backup" : "primary");

	if(FLAG_GET_BOOLEAN(verificationFlags, VERIF_INVALID_HDR_SIZE))
	{
		faultyHeader->HdrSize = validHeader->HdrSize;
		printf("[FAT32] [PARTMGR] [INFO]: Recovered HdrSize field from %s GPT to: %u\n", recoverBackup ? "primary" : "backup", validHeader->HdrSize);
	}

	if(FLAG_GET_BOOLEAN(verificationFlags, VERIF_INVALID_CURR_LBA))
	{
		faultyHeader->CurrentLBA = validHeader->BackupLBA;
		printf("[FAT32] [PARTMGR] [INFO]: Recovered CurrentLBA field from %s GPT to: %lu\n", recoverBackup ? "primary" : "backup", validHeader->BackupLBA);
	}

	if(FLAG_GET_BOOLEAN(verificationFlags, VERIF_INVALID_USABLE_LBA_START))
	{
		faultyHeader->UsableLBAStart = validHeader->UsableLBAStart;
		printf("[FAT32] [PARTMGR] [INFO]: Recovered UsableLBAStart field from %s GPT to: %lu\n", recoverBackup ? "primary" : "backup", validHeader->UsableLBAStart);
	}

	if(FLAG_GET_BOOLEAN(verificationFlags, VERIF_INVALID_USABLE_LBA_END))
	{
		faultyHeader->UsableLBAEnd = validHeader->UsableLBAEnd;
		printf("[FAT32] [PARTMGR] [INFO]: Recovered UsableLBAEnd field from %s GPT to: %lu\n", recoverBackup ? "primary" : "backup", validHeader->UsableLBAEnd);
	}

	if(FLAG_GET_BOOLEAN(verificationFlags, VERIF_INVALID_PART_ENTRY_SIZE))
	{
		faultyHeader->PartEntrySize = validHeader->PartEntrySize;
		printf("[FAT32] [PARTMGR] [INFO]: Recovered PartEntrySize field from %s GPT to: %u\n", recoverBackup ? "primary" : "backup", validHeader->PartEntrySize);
	}

	if(FLAG_GET_BOOLEAN(verificationFlags, VERIF_INVALID_PART_ENTRY_COUNT))
	{
		faultyHeader->PartEntryCount = validHeader->PartEntryCount;
		printf("[FAT32] [PARTMGR] [INFO]: Recovered PartEntryCount field from %s GPT to: %u\n", recoverBackup ? "primary" : "backup", validHeader->PartEntryCount);
	}

	if(FLAG_GET_BOOLEAN(verificationFlags, VERIF_INVALID_PART_ENTRY_START))
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot recover the %s GPT partEntryStartLBA field, as that would require either assumptions about that value, completely rewriting the partition table, or another type of recovery. The disk was not modified.\n", recoverBackup ? "backup" : "primary");
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Use GPT fdisk(gdisk) or another disk recovery tool to recover the GUID Partition Table\n");
		return FAT32_PARTITION_TABLE_RECOVERY_FAILED;
	}

	if(FLAG_GET_BOOLEAN(verificationFlags, VERIF_INVALID_PART_ENTRY_CRC32))
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot recover the %s GPT partEntryCRC32 field, because the partition table might be invalid, and this tool doesn't perform recovery on that. The disk was not modified.\n", recoverBackup ? "backup" : "primary");
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Use GPT fdisk(gdisk) or another disk recovery tool to recover the GUID Partition Table\n");
		return FAT32_PARTITION_TABLE_RECOVERY_FAILED;
	}

	// We already know that at least 1 of the fields weren't correct, and we changed it, so we need to recompute the header CRC32 anyway

	faultyHeader->HdrCRC32 = 0;
	faultyHeader->HdrCRC32 = ComputeCRC32(faultyHeader, faultyHeader->HdrSize);
	printf("[FAT32] [PARTMGR] [INFO]: Recomputed HdrCRC32 field in %s GPT to: %u\n", recoverBackup ? "backup" : "primary", faultyHeader->HdrCRC32);

	// Verify the recovered GPT just to make sure we don't write an invalid GPT Header
	auto verifRes = VerifyGPT(faultyHeader, recoverBackup, true);
	if(!verifRes)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to verify recovered %s GPT\n", recoverBackup ? "backup" : "primary");
		return verifRes.error();
	}
	if(verifRes.value() != 0)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: recovered %s GPT didn't pass verification. flags: 0x%lX\n", recoverBackup ? "backup" : "primary", verifRes.value());
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Use GPT fdisk(gdisk) or another disk recovery tool to recover the GUID Partition Table\n");
		return FAT32_PARTITION_TABLE_RECOVERY_FAILED;
	}

	// For safety ask the user whether to write the recovered primary GPT to the disk
	fprintf(stderr, "[FAT32] [PARTMGR] [INFO]: %s GPT Recovery succeeded. Currently changes have only been made in the memory.\n", recoverBackup ? "Backup" : "Primary");
	fprintf(stderr, "[FAT32] [PARTMGR] [WARN]: Writing the changes to the disk could result in loss of data\n");
	while(true)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [INFO]: Do you want to write the changes to the disk? Y/n: ");

		char buffer[16];
		fgets(buffer, sizeof(buffer), stdin);
		choice = toupper(buffer[0]);
		if(choice == 'N')
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Use GPT fdisk(gdisk) or another disk recovery tool to recover the GUID Partition Table\n");
			return FAT32_PARTITION_TABLE_RECOVERY_DENIED_BY_USER;
		}
		if(choice == 'Y') break;
		else
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Unknown choice %c, please try again.\n", choice);
			continue;
		}
	}


	printf("[FAT32] [PARTMGR] [INFO]: Writing the recovered %s GPT to the disk at LBA %lu\n", recoverBackup ? "backup" : "primary", recoverBackup ?faultyHeader->BackupLBA : 1UL);
	status = disk->WriteSectors(validHeader->BackupLBA, 1, faultyHeader);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to write the recovered %s GPT to the disk, lba: %lu, count: 1\n", recoverBackup ? "backup" : "primary", recoverBackup ? 1UL : faultyHeader->BackupLBA);
		return status;
	}

	return FAT32_PARTITION_TABLE_RECOVERY_SUCCEEDED;
}

std::expected<PartDesc*, FAT32_STATUS> PartMgr::OpenPartitionByIndex(uint32_t partIndex)
{
	if(partIndex == 0)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot open a partition with index %u, partition indexes start from 1\n", partIndex);
		return std::unexpected(FAT32_INVALID_PARTITION_INDEX);
	}

	PartDesc partDesc = {};
	FAT32_STATUS status;

	if(this->tableType == PartTableType::PART_TYPE_GPT)
	{
		// Get the partition entry
		auto openRes = OpenGPTPartitionEntry(partIndex);
		if(!openRes)
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to open GPT Partition at index %u, disk GUID: " GUID_PRINT_FORMAT "\n", partIndex, DECODE_GUID_TO_ARG(this->diskGUID));
			return std::unexpected(openRes.error());
		}

		status = OpenGPTPartition(&openRes.value(), &partDesc, partIndex);
		if(FAT32_ERROR(status)) 
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to open GPT Partition with index %u, disk GUID: " GUID_PRINT_FORMAT "\n", partIndex, DECODE_GUID_TO_ARG(this->diskGUID));
			return std::unexpected(status);
		}

		PartDesc* partDescPtr = reinterpret_cast<PartDesc*>(malloc(sizeof(PartDesc)));
		if(!partDescPtr)
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to allocate memory for the PartDesc structure\n");
			return std::unexpected(FAT32_MEMORY_ALLOCATION_FAILED);
		}

		memcpy(partDescPtr, &partDesc, sizeof(PartDesc));
		return partDescPtr;
	}

	// Partition is MBR

	MBR_Header* mbr = reinterpret_cast<MBR_Header*>(this->table);

	// Macro for opening the partition and returning the pointer
	auto openPart = [&](MBR_PartitionEntry* entry, uint32_t partIdx) -> std::expected<PartDesc*, FAT32_STATUS>
	{
		status = OpenMBRPartition(entry, &partDesc, partIdx);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to open MBR Partition with index %u, disk ID: %u\n", partIndex, this->diskID);
			return std::unexpected(status);
		}

		PartDesc* partDescPtr = reinterpret_cast<PartDesc*>(malloc(sizeof(PartDesc)));
		if(!partDescPtr)
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to allocate memory for the PartDesc structure\n");
			return std::unexpected(FAT32_MEMORY_ALLOCATION_FAILED);
		}

		memcpy(partDescPtr, &partDesc, sizeof(PartDesc));
		return partDescPtr;
	};

	// Check if there is an extended partition and see if the partition is in the MBR or some EBR

	uint8_t extendedPartIndex = 5;
	for(size_t i = 0; i < 4; i++)
	{
		if(IsExtendedPartition(&mbr->partEntries[i]))
		{
			extendedPartIndex = i + 1;
			break;
		}
	}

	if(extendedPartIndex == 5)
	{
		// No extended partition
		// Check if the partIndex isn't above 4
		if(partIndex > 4)
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Partition Index %u is outside the partition range, disk ID: %u\n", partIndex, this->diskID);
			return std::unexpected(FAT32_INVALID_PARTITION_INDEX);
		}
		return openPart(&mbr->partEntries[partIndex - 1], partIndex);
	}

	// An extended partition is present, check if the partition index is below the extended partition or is the same or above the extended partition
	if(partIndex < extendedPartIndex)
	{
		// Partition to open is a primary partition, safe to use partIndex to access it
		return openPart(&mbr->partEntries[partIndex - 1], partIndex);
	}

	// Go through each EBR until the current index matches the index requested
	uint32_t currentIndex = extendedPartIndex;
	MBR_PartitionEntry nextEBREntry = mbr->partEntries[extendedPartIndex - 1];
	uint32_t extendedPartitions = 1;
	while(currentIndex <= partIndex)
	{
		MBR_ExtendedPartition ebr = {};
		if(nextEBREntry.partitionType == 0x00) break;
		if(nextEBREntry.partitionStartLBA == 0)
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Invalid Extended Partition %u, disk ID: %u\n", currentIndex, this->diskID);
			return std::unexpected(FAT32_INVALID_PARTITION_TABLE);
		}

		status = disk->ReadSectors(nextEBREntry.partitionStartLBA, 1, &ebr);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to read next EBR from DISK, lba: %u, count: 1, disk ID: %u\n", nextEBREntry.partitionStartLBA, this->diskID);
			return std::unexpected(FAT32_DISK_READ_ERROR);
		}

		if(currentIndex == partIndex)
		{
			// Found the EBR containing the logical partition, open it
			return openPart(&ebr.entries[0], partIndex);
		}

		// Store the next EBR entry
		nextEBREntry = ebr.entries[1];
		extendedPartitions++;
		currentIndex++;
	}

	// partition is not a logical one, check the rest of the primary partitions
	uint32_t tmpPartIndex = partIndex - extendedPartitions;
	if(tmpPartIndex > 4)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Partition Index %u is outside the partition range, disk ID: %u\n", partIndex, this->diskID);
		return std::unexpected(FAT32_INVALID_PARTITION_INDEX);
	}

	return openPart(&mbr->partEntries[tmpPartIndex - 1], tmpPartIndex);
}

std::expected<PartDesc*, FAT32_STATUS> PartMgr::OpenPartitionByGUID(FAT32_GUID partGUID)
{
	if(this->tableType != PartTableType::PART_TYPE_GPT)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot open a partition using a GUID on a non-GPT disk\n");
		return std::unexpected(FAT32_UNSUPPORTED);
	}

	if(partGUID == emptyGUID)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot open a partition with a zero GUID\n");
		return std::unexpected(FAT32_INVALID_PARAMETER);
	}

	GPT_Header* gptHeader = reinterpret_cast<GPT_Header*>(this->table);
	PartDesc partDesc = {};
	FAT32_STATUS status;

	for(size_t i = 0; i < static_cast<size_t>(gptHeader->PartEntryCount); i++)
	{
		auto openRes = OpenGPTPartitionEntry(i + 1);
		if(!openRes)
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to open GPT Partition Entry at index %lu, disk GUID: " GUID_PRINT_FORMAT "\n", i + 1, DECODE_GUID_TO_ARG(this->diskGUID));
			return std::unexpected(openRes.error());
		}

		GPT_PartitionEntry entry = openRes.value();

		if(entry.UniqueIdentifier == partGUID)
		{
			status = OpenGPTPartition(&entry, &partDesc, i);
			if(FAT32_ERROR(status)) 
			{
				fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to open GPT Partition with index %lu, disk GUID: " GUID_PRINT_FORMAT "\n", i + 1, DECODE_GUID_TO_ARG(this->diskGUID));
				return std::unexpected(status);
			}

			PartDesc* partDescPtr = reinterpret_cast<PartDesc*>(malloc(sizeof(PartDesc)));
			if(!partDescPtr)
			{
				fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to allocate memory for the PartDesc structure\n");
				return std::unexpected(FAT32_MEMORY_ALLOCATION_FAILED);
			}

			memcpy(partDescPtr, &partDesc, sizeof(PartDesc));
			return partDescPtr;
		}
	}

	fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Couldn't find a GPT Partition with GUID: " GUID_PRINT_FORMAT "\n", DECODE_GUID_TO_ARG(this->diskGUID));
	return std::unexpected(FAT32_NOT_FOUND);
}

std::expected<GPT_PartitionEntry, FAT32_STATUS> PartMgr::OpenGPTPartitionEntry(uint32_t partitionIndex)
{
	if(partitionIndex == 0)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Invalid partition index %u, partition indexes start from 1\n", partitionIndex);
		return std::unexpected(FAT32_INVALID_PARTITION_INDEX);
	}

	uint8_t sectorBuffer[SECTOR_SIZE];
	FAT32_STATUS status;
	GPT_Header* gptHeader = reinterpret_cast<GPT_Header*>(this->table);

	if(partitionIndex > gptHeader->PartEntryCount)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot open partition with index %u: index out of total partition count, disk GUID: " GUID_PRINT_FORMAT "\n", partitionIndex, DECODE_GUID_TO_ARG(this->diskGUID));
		return std::unexpected(FAT32_INVALID_PARTITION_INDEX);
	}

	uint64_t partEntryOffset = gptHeader->PartEntryStartLBA * SECTOR_SIZE + (partitionIndex - 1) * gptHeader->PartEntrySize;
	uint64_t partEntrySector = OFFSET_TO_SECTOR(partEntryOffset);
	uint64_t partEntryOffsetInSector = OFFSET_IN_SECTOR(partEntryOffset);
	uint64_t bytesInCurrentSector = std::min<uint64_t>(SECTOR_SIZE - partEntryOffsetInSector, gptHeader->PartEntrySize);
	bytesInCurrentSector = std::min(bytesInCurrentSector, sizeof(GPT_PartitionEntry)); // Limit size to 128 bytes to prevent buffer overflows
	bool crossesSectorBoundary = bytesInCurrentSector < gptHeader->PartEntrySize;

	status = disk->ReadSectors(partEntrySector, 1, sectorBuffer);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to read partition entry from DISK, lba: %lu, count: 1, disk GUID: " GUID_PRINT_FORMAT "\n", partEntrySector, DECODE_GUID_TO_ARG(this->diskGUID));
		return std::unexpected(status);
	}

	GPT_PartitionEntry partitionEntry = {};

	memcpy(&partitionEntry, sectorBuffer + partEntryOffsetInSector, bytesInCurrentSector);

	// The GPT Partition entry might cross the sector boundary if the partition entry size is not a power of 2, so we need to handle that by doing a second read and copying the rest of the data
	if(crossesSectorBoundary)
	{
		status = disk->ReadSectors(partEntrySector + 1, 1, sectorBuffer);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Failed to read the rest of the partition entry from DISK, lba: %lu, count: 1, disk GUID: " GUID_PRINT_FORMAT "\n", partEntrySector + 1, DECODE_GUID_TO_ARG(this->diskGUID));
			return std::unexpected(status);
		}
		uint64_t bytesLeft = sizeof(GPT_PartitionEntry) - bytesInCurrentSector;
		memcpy(reinterpret_cast<uint8_t*>(&partitionEntry) + bytesInCurrentSector, sectorBuffer, bytesLeft);
	}

	return partitionEntry;
}

FAT32_STATUS PartMgr::OpenGPTPartition(GPT_PartitionEntry* entry, PartDesc* desc, uint32_t partitionIndex)
{
	// Check if the entry type is an empty GUID
	if(entry->PartType == emptyGUID)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot open an empty GPT Partition\n");
		return FAT32_INVALID_PARTITION_INDEX;
	}

	// Check if the entry start LBA is above the end LBA
	if(entry->StartLBA > entry->EndLBA)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Partition Start LBA is above the End LBA\n");
		return FAT32_INVALID_PARTITION_ENTRY;
	}

	// Check if the entry end LBA is outside the disk
	if(entry->EndLBA >= this->diskSectorCount)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Partition End LBA is above the disk sector count\n");
		return FAT32_INVALID_PARTITION_ENTRY;
	}

	GPT_Header* primaryHeader = reinterpret_cast<GPT_Header*>(this->table);
	GPT_Header* backupHeader = reinterpret_cast<GPT_Header*>(this->backupTable);

	uint64_t primaryPartitionTableEndLBA = primaryHeader->CurrentLBA + SECTOR_COUNT(static_cast<uint64_t>(primaryHeader->PartEntrySize) * primaryHeader->PartEntryCount);

	// Check if the Start LBA overlaps with the primary GPT or backup GPT Partition table
	if(entry->StartLBA <= primaryPartitionTableEndLBA)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Partition Start LBA overlaps with the primary GPT\n");
		return FAT32_INVALID_PARTITION_ENTRY;
	}

	// Check if the Start LBA overlaps with the backup GPT partition table
	if(entry->StartLBA >= backupHeader->PartEntryStartLBA)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Partition Start LBA overlaps with the backup GPT\n");
		return FAT32_INVALID_PARTITION_ENTRY;
	}

	// Check if the End LBA overlaps with the backup GPT partition table
	if(entry->EndLBA >= backupHeader->PartEntryStartLBA)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Partition End LBA overlaps with the backup GPT\n");
		return FAT32_INVALID_PARTITION_ENTRY;
	}

	// Fill in the Partition descriptor

	desc->diskGUID = this->diskGUID;
	desc->partIndexInDisk = partitionIndex;
	desc->partStartLBA = entry->StartLBA;
	desc->partEndLBA = entry->EndLBA;
	desc->readonly = FLAG_GET_BOOLEAN(entry->Flags, (1ULL << 60));
	return FAT32_SUCCESS;
}

FAT32_STATUS PartMgr::OpenMBRPartition(MBR_PartitionEntry* entry, PartDesc* desc, uint32_t partitionIndex)
{
	// Check if the partition is an empty partition
	if(entry->partitionType == 0x00)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot open an empty partition at index %u\n", partitionIndex);
		return FAT32_INVALID_PARTITION_INDEX;
	}

	// Check if the drive attributes are correct
	if(entry->driveAttribs != 0x00 && entry->driveAttribs != 0x80) fprintf(stderr, "[FAT32] [PARTMGR] [WARN]: MBR Partition %u has an invalid attribute value\n", partitionIndex);

	// Check if the partition sectors is 0
	if(entry->partitionSectors == 0)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Partition %u entry partitionSectors value is 0\n", partitionIndex);
		return FAT32_INVALID_PARTITION_ENTRY;
	}

	uint64_t endLBA = static_cast<uint64_t>(entry->partitionStartLBA) + entry->partitionSectors;

	// Check if the end LBA is outside the disk
	if(endLBA > this->diskSectorCount)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Partition %u end is outside the disk\n", partitionIndex);
		return FAT32_INVALID_PARTITION_ENTRY;
	}

	// Check if the partition overlaps with the MBR
	if(entry->partitionStartLBA == 0)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Partition %u StartLBA is 0\n", partitionIndex);
		return FAT32_INVALID_PARTITION_ENTRY;
	}

	// Make sure partition isn't an extended partition
	if(IsExtendedPartition(entry))
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot open an extended partition, partition index: %u\n", partitionIndex);
		return FAT32_INVALID_PARTITION_ENTRY;
	}

	// Fill in the Partition descriptor

	desc->diskID = this->diskID;
	desc->partIndexInDisk = partitionIndex;
	desc->partStartLBA = entry->partitionStartLBA;
	desc->partEndLBA = endLBA;
	return FAT32_SUCCESS;
}

FAT32_STATUS PartMgr::ReadSectors(PartDesc* desc, uint64_t lba, size_t count, void* bufferOut)
{
	if(!desc || count == 0 || !bufferOut)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Invalid ReadSectors input parameters\n");
		return FAT32_INVALID_PARAMETER;
	}

	if(this->tableType == PartTableType::PART_TYPE_GPT)
	{
		if(desc->diskGUID != this->diskGUID)
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot read from a partition for a different disk than the one this Partition Manager instance uses\n");
			return FAT32_INVALID_PARAMETER;
		}
	}
	else if(this->tableType == PartTableType::PART_TYPE_MBR)
	{
		if(desc->diskID != this->diskID)
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot read from a partition for a different disk than the one this Partition Manager instance uses\n");
			return FAT32_INVALID_PARAMETER;
		}
	}

	if(lba + count > desc->partEndLBA - desc->partStartLBA)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot read sectors outside of the partition\n");
		return FAT32_INVALID_PARAMETER;
	}

	return disk->ReadSectors(desc->partStartLBA + lba, count, bufferOut);
}

FAT32_STATUS PartMgr::WriteSectors(PartDesc* desc, uint64_t lba, size_t count, void* buffer)
{
	if(!desc || count == 0 || !buffer)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Invalid WriteSectors input parameters\n");
		return FAT32_INVALID_PARAMETER;
	}

	if(desc->readonly)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot write to a readonly partition\n");
		return FAT32_INVALID_PARAMETER;
	}

	if(this->tableType == PartTableType::PART_TYPE_GPT)
	{
		if(desc->diskGUID != this->diskGUID)
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot write to a partition for a different disk than the one this Partition Manager instance uses\n");
			return FAT32_INVALID_PARAMETER;
		}
	}
	else if(this->tableType == PartTableType::PART_TYPE_MBR)
	{
		if(desc->diskID != this->diskID)
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot write to a partition for a different disk than the one this Partition Manager instance uses\n");
			return FAT32_INVALID_PARAMETER;
		}
	}

	if(lba + count > desc->partEndLBA - desc->partStartLBA)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot write sectors outside of the partition\n");
		return FAT32_INVALID_PARAMETER;
	}

	return disk->WriteSectors(desc->partStartLBA + lba, count, buffer);
}

FAT32_STATUS PartMgr::ClosePartition(PartDesc* desc)
{
	if(!desc)
	{
		fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Invalid ClosePartition input parameters\n");
		return FAT32_INVALID_PARAMETER;
	}

	if(this->tableType == PartTableType::PART_TYPE_GPT)
	{
		if(desc->diskGUID != this->diskGUID)
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot close a partition from a different disk than the one this Partition Manager instance uses\n");
			return FAT32_INVALID_PARAMETER;
		}
	}
	else if(this->tableType == PartTableType::PART_TYPE_MBR)
	{
		if(desc->diskID != this->diskID)
		{
			fprintf(stderr, "[FAT32] [PARTMGR] [ERROR]: Cannot close a partition from a different disk than the one this Partition Manager instance uses\n");
			return FAT32_INVALID_PARAMETER;
		}
	}

	free(desc);
	return FAT32_SUCCESS;
}

PartMgr::~PartMgr()
{
	if(this->table) free(table);
	if(this->backupTable) free(backupTable);
}