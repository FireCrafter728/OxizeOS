// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdio>
#include <cstdint>

#include <unordered_map>

typedef int16_t WCHAR;

struct __attribute__((packed)) GUID
{
	uint32_t Data1;
	uint16_t Data2;
	uint16_t Data3;
	uint8_t Data4[8];
};

static constexpr GUID GUID_MS_BASIC_DATA = {0xEBD0A0A2, 0xB9E5, 0x4433, {0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7}};

static constexpr GUID GUID_LINUX_FILESYSTEM = {0x0FC63DAF, 0x8483, 0x4772, {0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7E, 0xE4}};

static constexpr GUID GUID_EFI_SYSTEM = {0xC12A7328, 0xF81F, 0x11D2, {0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B}};

struct StringHash
{
	size_t operator()(const std::string& s) const noexcept
	{
		return std::hash<std::string>{}(s);
	}
};

extern std::unordered_map<std::string, GUID, StringHash> GPTTypeMap;

struct __attribute__((packed)) MBR_PartitionEntry
{
	uint8_t attribs;
	uint8_t partStartCHS[3];
	uint8_t partType;
	uint8_t lastSectorCHS[3];
	uint32_t partStartLBA, partSectorsLBA;
};

struct __attribute__((packed)) MBR
{
	uint8_t bootCode[440];
	uint32_t DiskSignature;
	uint16_t _Reserved;
	MBR_PartitionEntry partitions[4];
	uint16_t bootSig;
};

struct __attribute__((packed)) GPT_Header
{
	uint8_t signature[8];
	uint32_t Revision;
	uint32_t HeaderSize;
	uint32_t HeaderCRC32;
	uint32_t _Reserved;
	uint64_t HeaderLBA;
	uint64_t BackupHeaderLBA;
	uint64_t FirstUsableLBA, LastUsableLBA;
	GUID DiskGUID;
	uint64_t PartTableLBA;
	uint32_t partEntryCount;
	uint32_t partEntrySize;
	uint32_t partTableCRC32;
	uint8_t _Reserved2[512 - 0x5C];
};

struct __attribute__((packed)) GPT_PartitionEntry
{
	GUID PartTypeGUID;
	GUID UniqueGUID;
	uint64_t StartLBA, EndLBA;
	uint64_t attribs;
	WCHAR PartitionName[36]; // -fshort-wchar specified
};

struct __attribute__((packed)) GPT_Table
{
	MBR ProtectiveMBR;
	GPT_Header header;
	GPT_PartitionEntry partitions[128];
};

struct __attribute__((packed)) BackupGPT
{
	GPT_PartitionEntry partitions[128];
	GPT_Header header;
};

class GPT
{
public:
	bool CreateGPT(FILE* image);
	bool CreatePart(FILE* image, GPT_PartitionEntry partEntry, uint8_t index);
	GUID GenerateGUID();
private:
	const uint8_t GPTHeaderSignature[8] = {'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T'};
	uint32_t ComputeCRC32(const void* data, size_t length);
};