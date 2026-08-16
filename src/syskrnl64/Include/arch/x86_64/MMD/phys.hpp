// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>
#include <SysTable.hpp>

#include <arch/x86_64/std/stdint.hpp>
#include <expected>

#include <arch/x86_64/MMD/memdefs.hpp>

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

		MemoryAllocErrors Initialize(SystemTable* System);
		std::expected<uintptr_t, MemoryAllocErrors> AllocContiguousBlocks(size_t blockCount);
		MemoryAllocErrors AllocSparseBlocksToContiguousVirtualRange(size_t blockCount, uintptr_t virt, uint64_t pageFlags);
		bool FreeBlocks(void* base, size_t blockCount);
	private:
		uint8_t* bitmap;
		size_t bitmapPageCount;
		uintptr_t lastFreeBlockOffset; // 64-bit aligned
		PA_MemoryRange memoryRanges[MAX_MEMORY_RANGES];

		uintptr_t BitOffsetToAddr(uint64_t bitOffset);
		uint64_t AddrToBitOffset(uintptr_t addr);
	};
}