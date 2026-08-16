// SPDX-License-Identifier: GPL-3.0-or-later

#include <gpt.hpp>

std::unordered_map<std::string, GUID, StringHash> GPTTypeMap = {
	{"0700", GUID_MS_BASIC_DATA},
	{"8300", GUID_LINUX_FILESYSTEM},
	{"EF00", GUID_EFI_SYSTEM}
};

bool GPT::CreateGPT(FILE* image)
{
	GPT_Table* gpt = (GPT_Table*)malloc(sizeof(GPT_Table));

	if(!gpt) {
		fprintf(stderr, "Failed to allocate memory for GPT structures\n");
		return false;
	}

	memset(gpt, 0, sizeof(GPT_Table));

	fseeko(image, 0, SEEK_END);
	size_t imageSize = ftello(image);
	fseeko(image, 0, SEEK_SET);

	size_t imageSectors = imageSize / SECTOR_SIZE;

	// Setup Protective MBR

	gpt->ProtectiveMBR.bootSig = 0xAA55;
	gpt->ProtectiveMBR.partitions[0].attribs = 0;
	gpt->ProtectiveMBR.partitions[0].partStartCHS[1] = 0x02;
	gpt->ProtectiveMBR.partitions[0].partType = 0xEE;
	memset(gpt->ProtectiveMBR.partitions[0].lastSectorCHS, 0xFF, 3);
	gpt->ProtectiveMBR.partitions[0].partStartLBA = 1;
	gpt->ProtectiveMBR.partitions[0].partSectorsLBA = 0xFFFFFFFF;

	// Setup main GPT Header

	memcpy(gpt->header.signature, GPTHeaderSignature, 8);
	gpt->header.Revision = 0x00010000;
	gpt->header.HeaderSize = 0x5C;
	gpt->header.HeaderLBA = 1;
	gpt->header.BackupHeaderLBA = imageSectors - 1;
	gpt->header.FirstUsableLBA = sizeof(GPT_Table) / SECTOR_SIZE;
	gpt->header.LastUsableLBA = imageSectors - sizeof(BackupGPT) / SECTOR_SIZE - 1;
	gpt->header.DiskGUID = GenerateGUID();
	gpt->header.PartTableLBA = 2;
	gpt->header.partEntryCount = 128;
	gpt->header.partEntrySize = 128;
	gpt->header.partTableCRC32 = 0xAB54D286;
	gpt->header.HeaderCRC32 = ComputeCRC32(&gpt->header, 0x5C);

	if(fwrite(gpt, 1, sizeof(GPT_Table), image) != sizeof(GPT_Table)) {
		fprintf(stderr, "Failed to write GPT to disk\n");
		return false;
	}

	BackupGPT* bGPT = (BackupGPT*)malloc(sizeof(BackupGPT));
	if(!bGPT) {
		fprintf(stderr, "Failed to allocate memory for backup GPT structures\n");
		return false;
	}

	memcpy(&bGPT->header, &gpt->header, sizeof(GPT_Header));
	memset(&bGPT->partitions, 0, sizeof(GPT_PartitionEntry) * 128);

	uint64_t tmp = bGPT->header.HeaderLBA;
	bGPT->header.HeaderLBA = bGPT->header.BackupHeaderLBA;
	bGPT->header.BackupHeaderLBA = tmp;
	bGPT->header.PartTableLBA = imageSectors - 33;
	bGPT->header.HeaderCRC32 = 0;
	bGPT->header.HeaderCRC32 = ComputeCRC32(&bGPT->header, 0x5C);

	fseeko(image, -(33ULL * SECTOR_SIZE), SEEK_END);

	if(fwrite(bGPT, 1, sizeof(BackupGPT), image) != sizeof(BackupGPT)) {
		fprintf(stderr, "Failed to write backup GPT to disk\n");
		return false;
	}

	return true;
}

bool GPT::CreatePart(FILE* image, GPT_PartitionEntry partEntry, uint8_t index)
{
	GPT_Table* gpt = (GPT_Table*)malloc(sizeof(GPT_Table));

	if(!gpt) {
		fprintf(stderr, "Failed to allocate memory for GPT structures\n");
		return false;
	}

	fseeko(image, 0, SEEK_SET);

	if(fread(gpt, 1, sizeof(GPT_Table), image) != sizeof(GPT_Table)) {
		fprintf(stderr, "Failed to read GPT\n");
		return false;
	}

	memcpy(&gpt->partitions[index - 1], &partEntry, sizeof(GPT_PartitionEntry));

	gpt->header.partTableCRC32 = ComputeCRC32(&gpt->partitions, sizeof(GPT_PartitionEntry) * 128);
	gpt->header.HeaderCRC32 = 0;
	gpt->header.HeaderCRC32 = ComputeCRC32(&gpt->header, 0x5C);

	fseeko(image, 0, SEEK_SET);

	if(fwrite(gpt, 1, sizeof(GPT_Table), image) != sizeof(GPT_Table)) {
		fprintf(stderr, "Failed to write GPT\n");
		return false;
	}

	BackupGPT* bGPT = (BackupGPT*)malloc(sizeof(GPT_Table));

	if(!bGPT) {
		fprintf(stderr, "Failed to allocate memory for backup GPT structures\n");
		return false;
	}

	fseeko(image, -(33ULL * SECTOR_SIZE), SEEK_END);

	if(fread(bGPT, 1, sizeof(BackupGPT), image) != sizeof(BackupGPT)) {
		fprintf(stderr, "Failed to read backup GPT\n");
		return false;
	}

	memcpy(&bGPT->partitions[index - 1], &partEntry, sizeof(GPT_PartitionEntry));

	bGPT->header.partTableCRC32 = ComputeCRC32(&bGPT->partitions, sizeof(GPT_PartitionEntry) * 128);

	bGPT->header.HeaderCRC32 = 0;
	bGPT->header.HeaderCRC32 = ComputeCRC32(&bGPT->header, 0x5C);

	fseeko(image, -(33ULL * SECTOR_SIZE), SEEK_END);

	if(fwrite(bGPT, 1, sizeof(BackupGPT), image) != sizeof(BackupGPT)) {
		fprintf(stderr, "Failed to write backup GPT\n");
		return false;
	}

	return true;
}

GUID GPT::GenerateGUID()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

	GUID guid;

	uint8_t data[16];

	uint32_t* p = reinterpret_cast<uint32_t*>(data);

	for(uint8_t i = 0; i < 4; i++) p[i] = dist(gen);

	guid.Data1 = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | (uint32_t)data[3];

	guid.Data2 = ((uint16_t)data[4] << 8) | (uint16_t)data[5];
	guid.Data3 = ((uint16_t)data[6] << 8) | (uint16_t)data[7];

	guid.Data4[0] = data[8];
	guid.Data4[1] = data[9];
	guid.Data4[2] = data[10];
	guid.Data4[3] = data[11];
	guid.Data4[4] = data[12];
	guid.Data4[5] = data[13];
	guid.Data4[6] = data[14];
	guid.Data4[7] = data[15];

	return guid;
}

uint32_t GPT::ComputeCRC32(const void* data, size_t length)
{
	static uint32_t table[256];
	static bool initialized = false;

	if(!initialized)
	{
		for(uint32_t i = 0; i < 256; i++)
		{
			uint32_t crc = i;
			for(int j = 0; j < 8; j++)
				crc = (crc >> 1) ^ (0xEDB88320u & (-(int)(crc & 1)));
			table[i] = crc;
		}
		initialized = true;
	}

	uint32_t crc = 0xFFFFFFFFu;
	const uint8_t* buf = static_cast<const uint8_t*>(data);

	for(size_t i = 0; i < length; i++)
	{
		crc = table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
	}

	return crc ^ 0xFFFFFFFFu;
}