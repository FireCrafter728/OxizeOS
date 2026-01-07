#include <gpt.hpp>

using namespace FAT32::GPT;

constexpr const GUID ESP_GUID = {0xC12A7328, 0xF81F, 0x11D2, {0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B}};

bool GPT::Initialize(DISK* disk, GPTDesc* desc, int Partition)
{
	this->desc = desc;
	this->disk = disk;

	if(!disk->ReadSectors(0, 34, desc))
	{
		printf("[GPT] [ERROR]: Failed to read GPT\r\n");
		return false;
	}

	if(Partition < 0)
	{
		for(int i = 0; i < GPT_MAX_PARTITIONS; i++)
		{
			if(memcmp(&desc->partitions[i].PartType, &ESP_GUID, sizeof(GUID)) == 0)
			{
				this->bootPartitionStart = desc->partitions[i].StartLBA;
				this->CurrentPartition = i + 1;
				break;
			}
		}

		if(this->bootPartitionStart == 0)
		{
			printf("[GPT] [ERROR]: Failed to find an ESP\r\n");
			return false;
		}
	}
	else if(Partition > GPT_MAX_PARTITIONS)
	{
		printf("[GPT] [ERROR]: Partition index %d is higher than maximum supported partitions\r\n", Partition);
		return false;
	}
	else {
		this->bootPartitionStart = desc->partitions[Partition - 1].StartLBA;
		this->CurrentPartition = Partition;
	}

	return true;
}

bool GPT::ReadSectors(m_uint32_t lba, m_uint16_t count, void* dataOut)
{
	return disk->ReadSectors(lba + this->bootPartitionStart, count, dataOut);
}

bool GPT::WriteSectors(m_uint32_t lba, m_uint16_t count, void* buffer)
{
	return disk->WriteSectors(lba + this->bootPartitionStart, count, buffer);
}

m_uint32_t GPT::GetPartitionStartLba(int partitionIndex)
{
	if(partitionIndex == -1) return desc->partitions[CurrentPartition - 1].StartLBA;
	return desc->partitions[partitionIndex - 1].StartLBA;
}

m_uint32_t GPT::GetPartitionSectorCount(int partitionIndex)
{
	if(partitionIndex == -1) return desc->partitions[CurrentPartition - 1].EndLBA - desc->partitions[CurrentPartition - 1].StartLBA;
	return desc->partitions[partitionIndex - 1].EndLBA - desc->partitions[partitionIndex - 1].StartLBA;
}