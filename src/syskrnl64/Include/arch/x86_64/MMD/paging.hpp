// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>
#include <arch/x86_64/std/stdint.hpp>
#include <SysTable.hpp>

namespace krnl
{
	enum PTE : uint64_t
	{
		PTE_PRESENT = (1ULL << 0),
		PTE_RW = (1ULL << 1ULL),
		PTE_USER = (1ULL << 2),
		PTE_PWT = (1ULL << 3),
		PTE_PCD = (1ULL << 4),
		PTE_ACCESSED = (1ULL << 5),
		PTE_DIRTY = (1ULL << 6), // PT only
		PTE_PAT = (1ULL << 7), // PT only
		PTE_GLOBAL = (1ULL << 8), // PT only
		PTE_NX = (1ULL << 63),

		PTE_PHYS_MASK = 0x000FFFFFFFFFF000,
	};

	#define MAKE_PTE(Addr, flags) (((Addr) & PTE_PHYS_MASK) | (flags))

	struct PACK Paging_FreeTableHeader
	{
		Paging_FreeTableHeader* next;
	};

	class Paging
	{
	public:
		Paging() = default;
		Paging(SystemTable* System);
		void Initialize(SystemTable* System);
		void MapArea(uintptr_t Phys, uintptr_t Virt, size_t pageCount, uint64_t flags);
		void FreeArea(uintptr_t Virt, size_t pageCount);
		uintptr_t GetKrnlStructVirt(uintptr_t Phys);
		uintptr_t GetPhys(uintptr_t Virt);
	private:
		uint64_t* AllocatePage();
		void FreePage(uintptr_t phys);
		size_t PageTablesPages;
		uintptr_t regionStart;
		uint64_t* PageTables;
		Paging_FreeTableHeader* freeTableList;
	};
}

constexpr uint64_t PTE_PRESENT = 	krnl::PTE_PRESENT;
constexpr uint64_t PTE_RW = 		krnl::PTE_RW;
constexpr uint64_t PTE_USER = 		krnl::PTE_USER;
constexpr uint64_t PTE_PWT = 		krnl::PTE_PWT;
constexpr uint64_t PTE_PCD = 		krnl::PTE_PCD;
constexpr uint64_t PTE_ACCESSED = 	krnl::PTE_ACCESSED;
constexpr uint64_t PTE_DIRTY = 		krnl::PTE_DIRTY;
constexpr uint64_t PTE_PAT = 		krnl::PTE_PAT;
constexpr uint64_t PTE_GLOBAL = 	krnl::PTE_GLOBAL;
constexpr uint64_t PTE_NX = 		krnl::PTE_NX;
constexpr uint64_t PTE_PHYS_MASK = 	krnl::PTE_PHYS_MASK;