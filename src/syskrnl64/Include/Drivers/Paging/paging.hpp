#pragma once
#include <stdint.hpp>
#include <SysTable.hpp>

#ifndef PACK
#define PACK __attribute__((packed))
#endif

#define PTE_PRESENT     SysKrnl64::Paging::PRESENT
#define PTE_RW          SysKrnl64::Paging::RW
#define PTE_USER        SysKrnl64::Paging::USER
#define PTE_WC          SysKrnl64::Paging::WC
#define PTE_CD          SysKrnl64::Paging::CD
#define PTE_ACCESSED    SysKrnl64::Paging::ACCESSED
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
            uintptr_t GetPhys(uintptr_t Virt);
            uintptr_t GetVirt(uintptr_t Phys);
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