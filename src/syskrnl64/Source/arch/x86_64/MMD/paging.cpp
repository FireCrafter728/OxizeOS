// SPDX-License-Identifier: GPL-3.0-or-later

#include <arch/x86_64/MMD/paging.hpp>

#include <arch/x86_64/Utility/io.hpp>
#include <stdio.hpp>
#include <string.hpp>

using namespace krnl;

Paging::Paging(SystemTable* System)
{
	Initialize(System);
}

void Paging::Initialize(SystemTable* System)
{
	this->PageTablesPages = System->memLayout.PageTablePageCount;
	this->regionStart = System->memLayout.regionStartPhys;
	this->PageTables = reinterpret_cast<uint64_t*>(System->memLayout.PageTableAddr);
	this->freeTableList = reinterpret_cast<Paging_FreeTableHeader*>(System->memLayout.NextPageTableFreePtr);
}

void Paging::MapArea(uintptr_t Phys, uintptr_t Virt, size_t pageCount, uint64_t flags)
{
	if(!this->freeTableList || !this->PageTables || !this->PageTablesPages || !this->regionStart) {
		printf("[SYSKRNL64] [PAGING] [ERROR]: MapArea() called when driver was not initialized\r\n");
		HaltSystem();
	}
	Virt = PAGE_ALIGN_DOWN(Virt);
	Phys = PAGE_ALIGN_DOWN(Phys);
	for(size_t page = 0; page < pageCount; page++)
	{
		// Get current page phys & virt addresses
		const uintptr_t currVirt = Virt + page * PAGE_SIZE;
		const uintptr_t currPhys = Phys + page * PAGE_SIZE;

		// Get page tables indices for current vaddr
		const uint16_t pml4_idx = (currVirt >> 39) & 0x1FF;
		const uint16_t pdpt_idx = (currVirt >> 30) & 0x1FF;
		const uint16_t pd_idx = (currVirt >> 21) & 0x1FF;
		const uint16_t pt_idx = (currVirt >> 12) & 0x1FF;

		// retrieve PDPT Table from PML4 table
		volatile uint64_t* pml4 = this->PageTables;
		volatile uint64_t* pdpt_entry = &pml4[pml4_idx];
		if(!(*pdpt_entry & PTE_PRESENT)) {
			// Entry doesn't exist, create it
			volatile uint64_t* pdpt_addr = AllocatePage();
			*pdpt_entry = MAKE_PTE(reinterpret_cast<uintptr_t>(pdpt_addr), PTE_PRESENT | PTE_RW);
		}
		volatile uint64_t* pdpt = reinterpret_cast<volatile uint64_t*>(GetKrnlStructVirt(*pdpt_entry & PTE_PHYS_MASK));
 
		// retrieve PD Table from PDPT table
		volatile uint64_t* pd_entry = &pdpt[pdpt_idx];
		if(!(*pd_entry & PTE_PRESENT)) {
			volatile uint64_t* pd_addr = AllocatePage();
			*pd_entry = MAKE_PTE(reinterpret_cast<uintptr_t>(pd_addr), PTE_PRESENT | PTE_RW);
		}
		volatile uint64_t* pd = reinterpret_cast<volatile uint64_t*>(GetKrnlStructVirt(*pd_entry & PTE_PHYS_MASK));

		// retrieve PT Table from PD table
		volatile uint64_t* pt_entry = &pd[pd_idx];
		if(!(*pt_entry & PTE_PRESENT)) {
			volatile uint64_t* pt_addr = AllocatePage();
			*pt_entry = MAKE_PTE(reinterpret_cast<uintptr_t>(pt_addr), PTE_PRESENT | PTE_RW);
		}
		volatile uint64_t* pt = reinterpret_cast<volatile uint64_t*>(GetKrnlStructVirt(*pt_entry & PTE_PHYS_MASK));

		// Create a new entry with respective flags in PT table
		volatile uint64_t* page_entry = &pt[pt_idx];
		*page_entry = MAKE_PTE(currPhys, flags);
		__asm__ volatile("" ::: "memory");
		InvalidatePage(currVirt);
	}
}

void Paging::FreeArea(uintptr_t Virt, size_t pageCount)
{
	Virt = PAGE_ALIGN_DOWN(Virt);

	for(size_t i = 0; i < pageCount; i++)
	{
		uintptr_t currVirt = Virt + i * PAGE_SIZE;

		const uint16_t pml4_idx = (currVirt >> 39) & 0x1FF;
		const uint16_t pdpt_idx = (currVirt >> 30) & 0x1FF;
		const uint16_t pd_idx = (currVirt >> 21) & 0x1FF;
		const uint16_t pt_idx = (currVirt >> 12) & 0x1FF;

		uint64_t* pml4 = this->PageTables;

		uint64_t pdpt_entry = pml4[pml4_idx];
		if(!(pdpt_entry & PTE_PRESENT)) continue;
		uint64_t* pdpt = reinterpret_cast<uint64_t*>(GetKrnlStructVirt(pdpt_entry & PTE_PHYS_MASK));

		uint64_t pd_entry = pdpt[pdpt_idx];
		if(!(pd_entry & PTE_PRESENT)) continue;
		uint64_t* pd = reinterpret_cast<uint64_t*>(GetKrnlStructVirt(pd_entry & PTE_PHYS_MASK));

		uint64_t pt_entry = pd[pd_idx];
		if(!(pt_entry & PTE_PRESENT)) continue;
		uint64_t* pt = reinterpret_cast<uint64_t*>(GetKrnlStructVirt(pt_entry & PTE_PHYS_MASK));

		uint64_t PageEntry = pt[pt_idx];
		if(PageEntry & PTE_PRESENT)
		{
			pt[pt_idx] = 0;
			InvalidatePage(currVirt);
			
			bool ptEmpty = true;
			for(size_t j = 0; j < 512; j++) if(pt[j] & PTE_PRESENT) {
				ptEmpty = false;
				break;
			}
			if(ptEmpty)
			{
				FreePage(GetPhys(reinterpret_cast<uintptr_t>(pt)));
				pd[pt_idx] = 0;

				bool pdEmpty = true;
				for(size_t j = 0; j < 512; j++) if(pd[j] & PTE_PRESENT) {
					pdEmpty = false;
					break;
				}
				if(pdEmpty) {
					FreePage(GetPhys(reinterpret_cast<uintptr_t>(pd)));
					pdpt[pdpt_idx] = 0;

					bool pdptEmpty = true;
					for(size_t j = 0; j < 512; j++) if(pdpt[j] & PTE_PRESENT) {
						pdptEmpty = false;
						break;
					}
					if(pdptEmpty)
					{
						FreePage(GetPhys(reinterpret_cast<uintptr_t>(pdpt)));
						pml4[pml4_idx] = 0;
					}
				}
			}
		}
	}
}

uint64_t* Paging::AllocatePage()
{
	if(!this->freeTableList) {
		printf("[SYSKRNL64] [PAGING] [ERROR]: AllocatePage() called when driver was not initialized\r\n");
		HaltSystem();
	}

	uintptr_t pagePhys = reinterpret_cast<uintptr_t>(this->freeTableList);
	Paging_FreeTableHeader* page = reinterpret_cast<Paging_FreeTableHeader*>(GetKrnlStructVirt(pagePhys));
	this->freeTableList = page->next;
	memset(page, 0, PAGE_SIZE);

	return reinterpret_cast<uint64_t*>(pagePhys);
}

void Paging::FreePage(uintptr_t phys)
{
	if(!phys) return;

	Paging_FreeTableHeader* vpage = reinterpret_cast<Paging_FreeTableHeader*>(GetKrnlStructVirt(phys));

	memset(vpage, 0, PAGE_SIZE);

	vpage->next = reinterpret_cast<Paging_FreeTableHeader*>(freeTableList);
	freeTableList = reinterpret_cast<Paging_FreeTableHeader*>(phys);
}

uintptr_t Paging::GetPhys(uintptr_t Virt)
{
	// Get page tables indices for current vaddr
	const uint16_t pml4_idx = (Virt >> 39) & 0x1FF;
	const uint16_t pdpt_idx = (Virt >> 30) & 0x1FF;
	const uint16_t pd_idx = (Virt >> 21) & 0x1FF;
	const uint16_t pt_idx = (Virt >> 12) & 0x1FF;
	const uint16_t virtOffset = Virt & 0xFFF;

	// retrieve PDPT Table from PML4 table
	volatile uint64_t* pml4 = this->PageTables;
	volatile uint64_t* pdpt_entry = &pml4[pml4_idx];
	if(!(*pdpt_entry & PTE_PRESENT)) {
		// Entry doesn't exist
		return 0;
	}
	volatile uint64_t* pdpt = reinterpret_cast<volatile uint64_t*>(GetKrnlStructVirt(*pdpt_entry & PTE_PHYS_MASK));
 
	// retrieve PD Table from PDPT table
	volatile uint64_t* pd_entry = &pdpt[pdpt_idx];
	if(!(*pd_entry & PTE_PRESENT)) {
		return 0;
	}
	volatile uint64_t* pd = reinterpret_cast<volatile uint64_t*>(GetKrnlStructVirt(*pd_entry & PTE_PHYS_MASK));

	// retrieve PT Table from PD table
	volatile uint64_t* pt_entry = &pd[pd_idx];
	if(!(*pt_entry & PTE_PRESENT)) {
		return 0;
	}
	volatile uint64_t* pt = reinterpret_cast<volatile uint64_t*>(GetKrnlStructVirt(*pt_entry & PTE_PHYS_MASK));

	// Return the physical address in the page entry
	volatile uint64_t* page_entry = &pt[pt_idx];

	return (*page_entry & PTE_PHYS_MASK) + virtOffset;
}

uintptr_t Paging::GetKrnlStructVirt(uintptr_t Phys)
{
	return Phys + MapAddr - this->regionStart;
}