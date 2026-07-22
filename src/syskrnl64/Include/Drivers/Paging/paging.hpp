// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |-----------------------------------------------------------------------------------------| //
// | OxizeOS Kernel Implementation                                                           | //
// | PAGING DRIVER: A driver for managing the PML4 page tables and the virtual address space | //
// |-----------------------------------------------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdint.hpp>
#include <SysTable.hpp>

#ifndef PACK
#define PACK __attribute__((packed))
#endif

#define PTE_PRESENT     SysKrnl64::Paging::PRESENT
#define PTE_RW          SysKrnl64::Paging::RW
#define PTE_USER        SysKrnl64::Paging::USER
#define PTE_PWT         SysKrnl64::Paging::PWT
#define PTE_PCD         SysKrnl64::Paging::PCD
#define PTE_ACCESSED    SysKrnl64::Paging::ACCESSED
#define PTE_DIRTY       SysKrnl64::Paging::DIRTY
#define PTE_PAT         SysKrnl64::Paging::PAT
#define PTE_GLOBAL      SysKrnl64::Paging::GLOBAL
#define PTE_NX          SysKrnl64::Paging::NX
#define PTE_PHYS_MASK   SysKrnl64::Paging::PHYS_MASK

namespace SysKrnl64
{
    namespace Paging
    {
        enum PTE : uint64_t
	    {
	    	PRESENT = (1ULL << 0),
	    	RW = (1ULL << 1ULL),
	    	USER = (1ULL << 2),
	    	PWT = (1ULL << 3),
	    	PCD = (1ULL << 4),
	    	ACCESSED = (1ULL << 5),
            DIRTY = (1ULL << 6), // PT only
            PAT = (1ULL << 7), // PT only
            GLOBAL = (1ULL << 8), // PT only
	    	NX = (1ULL << 63),

	    	PHYS_MASK = 0x000FFFFFFFFFF000,
	    };

	    #define MAKE_PTE(Addr, flags) (((Addr) & PTE_PHYS_MASK) | (flags))

        struct PACK FreeTableHeader
        {
            FreeTableHeader* next;
        };

        class Paging
        {
        public:
            Paging() = default;
            Paging(SystemTable* System);
            void Initialize(SystemTable* System);
            void MapArea(uintptr_t Phys, uintptr_t Virt, size_t pageCount, flags_t flags);
            void FreeArea(uintptr_t Virt, size_t pageCount);
            uintptr_t GetKrnlStructVirt(uintptr_t Phys);
            uintptr_t GetPhys(uintptr_t Virt);
        private:
            uint64_t* AllocatePage();
            void FreePage(uintptr_t phys);
            size_t PageTablesPages;
            uintptr_t regionStart;
            uint64_t* PageTables;
            FreeTableHeader* freeTableList;
        };
    }
}