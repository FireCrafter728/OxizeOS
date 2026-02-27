#pragma once
#include <stdint.hpp>
#include <SysTable.hpp>

#ifndef PACK
#define PACK __attribute__((packed))
#endif

namespace TskSchl
{
    namespace Paging
    {
        enum PTE : uint64_t
	    {
	    	PTE_PRESENT = (1ULL << 0),
	    	PTE_RW = (1ULL << 1ULL),
	    	PTE_USER = (1ULL << 2),
	    	PTE_WC = (1ULL << 3),
	    	PTE_CD = (1ULL << 4),
	    	PTE_ACCESSED = (1ULL << 5),
	    	PTE_NX = (1ULL << 63),

	    	PTE_PHYS_MASK = 0x000FFFFFFFFFF000,
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