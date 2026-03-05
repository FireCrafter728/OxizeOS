#pragma once
#include <stdint.hpp>
#include <SysTable.hpp>

#ifndef PACK
#define PACK __attribute__((packed))
#endif

#define PTE_PRESENT     TskSchl::Paging::PRESENT
#define PTE_RW          TskSchl::Paging::RW
#define PTE_USER        TskSchl::Paging::USER
#define PTE_WC          TskSchl::Paging::WC
#define PTE_CD          TskSchl::Paging::CD
#define PTE_ACCESSED    TskSchl::Paging::ACCESSED
#define PTE_NX          TskSchl::Paging::NX
#define PTE_PHYS_MASK   TskSchl::Paging::PHYS_MASK

namespace TskSchl
{
    namespace Paging
    {
        enum PTE : uint64_t
	    {
	    	PRESENT = (1ULL << 0),
	    	RW = (1ULL << 1ULL),
	    	USER = (1ULL << 2),
	    	WC = (1ULL << 3),
	    	CD = (1ULL << 4),
	    	ACCESSED = (1ULL << 5),
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