#include <Drivers/Paging/paging.hpp>

using namespace TskSchl::Paging;

#define GetPhys(Virt) ((Virt) - MapAddr + this->regionStart)
#define GetVirt(Phys) ((Phys) + MapAddr - this->regionStart)

Paging::Paging(SystemTable* System)
{
    Initialize(System);
}

void Paging::Initialize(SystemTable* System)
{
    this->System = System;
    this->PageTablesAddr = System->memLayout.PageTableAddr;
    this->PageTablesPages = System->memLayout.PageTablePageCount;
    this->OffsetInPageTables = System->memLayout.PageTableOffset;
    this->regionStart = System->memLayout.regionStartPhys;
}

void Paging::MapArea(uintptr_t Phys, uintptr_t Virt, size_t pageCount, flags_t flags)
{
    Virt = PAGE_ALIGN_DOWN(Virt);
    Phys = PAGE_ALIGN_DOWN(Phys);
    for(size_t page = 0; page < pageCount; page++)
    {
        // Get current page phys & virt addresses
        uintptr_t currVirt = Virt + page * PAGE_SIZE;
        uintptr_t currPhys = Phys + page * PAGE_SIZE;

        // Get page tables indices for current vaddr
        uint16_t pml4_idx = (currVirt >> 39) & 0x1FF;
        uint16_t pdpt_idx = (currVirt >> 30) & 0x1FF;
        uint16_t pd_idx = (currVirt >> 21) & 0x1FF;
        uint16_t pt_idx = (currVirt >> 12) & 0x1FF;

        // retrieve PDPT Table from PML4 table
        uint64_t* pml4 = reinterpret_cast<uint64_t*>(this->PageTablesAddr);
        uint64_t* pdpt_entry = &pml4[pml4_idx];
        if(!(*pdpt_entry & PTE_PRESENT)) {
            // Entry doesn't exist, create it
            *pdpt_entry = MAKE_PTE(GetPhys(this->PageTablesAddr + this->OffsetInPageTables), PTE_PRESENT | PTE_RW);
            this->OffsetInPageTables += PAGE_SIZE;
            memset((void*)GetVirt(*pdpt_entry & PTE_PHYS_MASK), 0, PAGE_SIZE);
        }
        uint64_t* pdpt = reinterpret_cast<uint64_t*>(GetVirt(*pdpt_entry & PTE_PHYS_MASK));
 
        // retrieve PD Table from PDPT table
        uint64_t* pd_entry = &pdpt[pdpt_idx];
        if(!(*pd_entry & PTE_PRESENT)) {
            *pd_entry = MAKE_PTE(GetPhys(this->PageTablesAddr + this->OffsetInPageTables), PTE_PRESENT | PTE_RW);
            this->OffsetInPageTables += PAGE_SIZE;
            memset((void*)GetVirt(*pd_entry & PTE_PHYS_MASK), 0, PAGE_SIZE);
        }
        uint64_t* pd = reinterpret_cast<uint64_t*>(GetVirt(*pd_entry & PTE_PHYS_MASK));

        // retrieve PT Table from PD table
        uint64_t* pt_entry = &pd[pd_idx];
        if(!(*pt_entry & PTE_PRESENT)) {
            *pt_entry = MAKE_PTE(GetPhys(this->PageTablesAddr + this->OffsetInPageTables), PTE_PRESENT | PTE_RW);
            this->OffsetInPageTables += PAGE_SIZE;
            memset((void*)GetVirt(*pt_entry & PTE_PHYS_MASK), 0, PAGE_SIZE);
        }
        uint64_t* pt = reinterpret_cast<uint64_t*>(GetVirt(*pt_entry & PTE_PHYS_MASK));

        // Create a new entry with respective flags in PT table
        uint64_t* page_entry = &pt[pt_idx];
        *page_entry = MAKE_PTE(currPhys, flags);
        InvalidatePage(currVirt);
    }
}