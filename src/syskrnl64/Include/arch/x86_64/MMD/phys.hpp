// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>
#include <SysTable.hpp>

#include <expected>
#include <mutex>

#define MAX_MEMORY_RANGES 256

namespace krnl
{
	struct PA_MemoryRange
	{
		uintptr_t start;
		size_t length;
		uintptr_t bitmapIndexBase;
	};

	class PhysAlloc
	{
	public:
		PhysAlloc() = default;
		// Delete copy / move constructors / assignments
		PhysAlloc(const PhysAlloc&) = delete;
		PhysAlloc& operator=(const PhysAlloc&) = delete;
		PhysAlloc(PhysAlloc&&) = delete;
		PhysAlloc& operator=(PhysAlloc&&) = delete;

		KRNL_STATUS Initialize(SystemTable* System);
		std::expected<uintptr_t, KRNL_STATUS> AllocContiguousBlocks(size_t blockCount);
		KRNL_STATUS AllocSparseBlocksToContiguousVirtualRange(size_t blockCount, uintptr_t virt, uint64_t pageFlags);
		bool FreeBlocks(void* base, size_t blockCount);
	private:
		uint8_t* bitmap;
		size_t bitmapPageCount;
		uintptr_t lastFreeBlockOffset; // 64-bit aligned
		PA_MemoryRange memoryRanges[MAX_MEMORY_RANGES];

		uintptr_t BitOffsetToAddr(uint64_t bitOffset);
		uint64_t AddrToBitOffset(uintptr_t addr);

		std::mutex pallocMutex;
	};
}