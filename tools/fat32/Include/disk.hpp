// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <cstdio>

#include <defs.hpp>

namespace FAT32
{
	class DISK
	{
	public:
		FAT32_STATUS Initialize(const char* DiskImage);
		FAT32_STATUS ReadSectors(uint64_t lba, size_t count, void* dataOut);
		FAT32_STATUS WriteSectors(uint64_t lba, size_t count, void* buffer);
		uint64_t GetDiskSectorCount();
		~DISK();
	private:
		FILE* disk;
	};
}