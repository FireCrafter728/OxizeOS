// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

#define SECTOR_SIZE 512UL

#define SECTOR_ALIGN_UP(bytes) (((bytes) + SECTOR_SIZE - 1) & ~(SECTOR_SIZE - 1))
#define SECTOR_ALIGN_DOWN(bytes) ((bytes) & ~(SECTOR_SIZE - 1))

#define SECTOR_COUNT(bytes) ((SECTOR_ALIGN_UP(bytes)) / SECTOR_SIZE)
#define OFFSET_TO_SECTOR(offset) ((SECTOR_ALIGN_DOWN(offset)) / SECTOR_SIZE)
#define OFFSET_IN_SECTOR(offset) ((offset) % SECTOR_SIZE)

enum FAT32_STATUS : uint32_t
{
    FAT32_SUCCESS = 0x00,
    
    FAT32_INVALID_PARAMETER = 0x01,
	FAT32_MEMORY_ALLOCATION_FAILED = 0x02,
	FAT32_UNSUPPORTED = 0x03,
	FAT32_NOT_FOUND = 0x04,
	FAT32_NOT_IMPLEMENTED = 0x05,
	FAT32_HOST_FILESYSTEM_ERROR = 0x06,

    FAT32_DISK_ERROR = 0x10,
    FAT32_DISK_READ_ERROR = 0x11,
    FAT32_DISK_WRITE_ERROR = 0x12,

	FAT32_INVALID_PARTITION_TABLE = 0x20,
	FAT32_CORRUPTED_BACKUP_PARTITION_TABLE = 0x21,
	FAT32_INCONSISTENT_PARTITION_TABLES = 0x22,
	FAT32_PARTITION_TABLE_VERIFICATION_FAILED = 0x23,
	FAT32_PARTITION_TABLE_RECOVERY_FAILED = 0x24,
	FAT32_PARTITION_TABLE_RECOVERY_DENIED_BY_USER = 0x25,
	FAT32_UNKNOWN_OR_NO_PARTITION_TABLE = 0x26,
	FAT32_INVALID_PARTITION_INDEX = 0x27,
	FAT32_INVALID_PARTITION_ENTRY = 0x28,
	FAT32_PARTITION_TABLE_RECOVERY_SUCCEEDED = 0x2F,

	FAT32_FAT_END_OF_DIRECTORY = 0x30,
	FAT32_FAT_NAME_TOO_LONG = 0x31,
	FAT32_FAT_ALREADY_EXISTS = 0x32,
	FAT32_FAT_INVALID_NAME = 0x33,
	FAT32_FAT_OUT_OF_SPACE = 0x34,
	FAT32_FAT_CORRUPTED_FILE_ALLOCATION_TABLE = 0x35,
};

#define FAT32_ERROR(status) ((status) != FAT32_SUCCESS)

#define PACK __attribute__((packed))

struct FAT32_GUID
{
	uint32_t Data1;
	uint16_t Data2;
	uint16_t Data3;
	uint8_t Data4[8];

	bool operator==(const FAT32_GUID& other)
	{
		return Data1 == other.Data1 && Data2 == other.Data2 && Data3 == other.Data3 && Data4[0] == other.Data4[0] && Data4[1] == other.Data4[1] && Data4[2] == other.Data4[2] && Data4[3] == other.Data4[3] && Data4[4] == other.Data4[4] && Data4[5] == other.Data4[5] && Data4[6] == other.Data4[6] && Data4[7] == other.Data4[7];
	}

	bool operator!=(const FAT32_GUID& other)
	{
		return Data1 != other.Data1 || Data2 != other.Data2 || Data3 != other.Data3 || Data4[0] != other.Data4[0] || Data4[1] != other.Data4[1] || Data4[2] != other.Data4[2] || Data4[3] != other.Data4[3] || Data4[4] != other.Data4[4] || Data4[5] != other.Data4[5] || Data4[6] != other.Data4[6] || Data4[7] != other.Data4[7];
	}
};

#define GUID_PRINT_FORMAT "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X"
#define DECODE_GUID_TO_ARG(guid) (guid).Data1, (guid).Data2, (guid).Data3, (guid).Data4[0], (guid).Data4[1], (guid).Data4[2], (guid).Data4[3], (guid).Data4[4], (guid).Data4[5], (guid).Data4[6], (guid).Data4[7]

constexpr FAT32_GUID emptyGUID = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

struct PACK CHS
{
	uint8_t head;
	uint8_t sectorCylinderLow;
	uint8_t cylinderHigh;
};

typedef uint32_t crc32_t;
typedef uint32_t codepoint_t;

#define FLAG_SET(bitfield, flag) ((bitfield) |= (flag))
#define FLAG_UNSET(bitfield, flag) ((bitfield) &= ~(flag))
#define FLAG_GET_BOOLEAN(bitfield, flag) (((bitfield) & (flag)) != 0)
#define FLAG_GET_VALUE(bitfield, mask, shift) (((bitfield) & (mask)) >> (shift))