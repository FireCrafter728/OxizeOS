// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>
#include <arch/x86_64/std/stdint.hpp>

#include <arch/x86_64/MMD/memdefs.hpp>
#include <arch/x86_64/MMD/virt.hpp>

namespace krnl
{
	constexpr uint64_t HEAP_ALIGNMENT = 16;
	constexpr uint64_t INITIAL_HEAP_SIZE = 16 * BLOCK_SIZE; // 64KiB initial heap
	constexpr uint64_t HEAP_RESERVE_BLOCKS = 0x10000000; // Reserve 1TiB virtual address space for the heap. Not all of it is backed by physical memory, but can be expanded on demand

	enum Heap_AllocationHeaderFlags
	{
		// Bit 0: is region free or not? 0: free, 1: used
		HEAP_FLAG_USED = (1ULL << 0),
		
		
	};

	struct PACK Heap_AllocationHeader
	{
		size_t allocSize;
		Heap_AllocationHeader* prev;
		Heap_AllocationHeader* next;
		uint64_t flags;
	};

	constexpr uint64_t MINIMAL_FREE_HEAP_SIZE = sizeof(Heap_AllocationHeader) + HEAP_ALIGNMENT;

	struct Heap_HeapAllocDesc
	{
		VirtAlloc* virtAlloc;
		PhysAlloc* physAlloc;
	};

	class HeapAlloc
	{
	public:
		HeapAlloc() = default;
		MemoryAllocErrors Initialize(Heap_HeapAllocDesc* desc);
		std::expected<void*, MemoryAllocErrors> AllocateBytes(size_t size);
		MemoryAllocErrors FreeBytes(void* ptr);
	private:
		MemoryAllocErrors ExpandHeap();
		uintptr_t heapVirtBase;
		size_t heapBlocks;
		Heap_HeapAllocDesc desc;
		Heap_AllocationHeader* lastHeader;
	};
}