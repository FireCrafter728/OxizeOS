#pragma once
#include <disk.hpp>

namespace FAT32
{
	namespace GPT
	{
		struct __attribute__((packed)) PartitionEntry
		{
			GUID PartType;
			GUID UniqueIdentifier;
			m_uint64_t StartLBA, EndLBA;
			m_uint64_t Flags;
			m_uint16_t PartitionNameUnicode[36];
		};

		struct __attribute__((packed)) GPTHeader
		{
			m_uint64_t Signature;
			m_uint32_t Revision;
			m_uint32_t HdrSize;
			m_uint32_t HdrCRC32;
			m_uint32_t _Reserved;
			m_uint64_t CurrentLBA;
			m_uint64_t BackupLBA;
			m_uint64_t UsableLBAStart;
			m_uint64_t UsableLBAEnd;
			GUID DiskGUID;
			m_uint64_t PartEntryStartLBA;
			m_uint32_t PartEntryCount;
			m_uint32_t PartEntrySize;
			m_uint32_t PartEntryCRC32;
			m_uint8_t Padding[420];
		};

		struct __attribute__((packed)) GPTDesc
		{
			m_uint8_t ProtectiveMBR[SECTOR_SIZE];
			GPTHeader header = {};
			PartitionEntry partitions[128] = {};
		};

		class GPT
		{
		public:
			bool Initialize(DISK* disk, GPTDesc* desc, int Partition = -1);
			bool ReadSectors(m_uint32_t lba, m_uint16_t count, void* dataOut);
			bool WriteSectors(m_uint32_t lba, m_uint16_t count, void* buffer);
			inline int GetPartitionIndex() const {return CurrentPartition;};
			m_uint32_t GetPartitionStartLba(int partitionIndex = -1);
			m_uint32_t GetPartitionSectorCount(int partitionIndex = -1);
		private:
			GPTDesc* desc = nullptr;
			DISK* disk;
			int CurrentPartition;
			m_uint64_t bootPartitionStart;
		};
	}
}