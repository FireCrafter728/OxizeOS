#include <SysTableBuilder.hpp>

using namespace BootMgr::SysTable;

SystemTable* SysTable::BuildSystemTable(size_t SysKrnl64PageCount, EFI_SYSTEM_TABLE* gSystem)
{
	// Get UEFI Memory map
	efiMemmap = nullptr;
	uint64_t memmapSize = 0, mapKey;
	size_t prevAllocSize = 0;
	uint32_t descVersion;
	
	lastStatus = gSystem->BootServices->GetMemoryMap(&memmapSize, efiMemmap, &mapKey, &entrySize, &descVersion);
	if(lastStatus != EFI_BUFFER_TOO_SMALL) return nullptr;

	memmapSize += 2 * entrySize;
	EFI_PHYSICAL_ADDRESS efiMemmapAddr = 0;
	lastStatus = gSystem->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, (memmapSize + 0xFFF) / 0x1000, &efiMemmapAddr);
	prevAllocSize = memmapSize;
	if(EFI_ERROR(lastStatus)) return nullptr;
	efiMemmap = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(efiMemmapAddr);

	while(true) {
		lastStatus = gSystem->BootServices->GetMemoryMap(&memmapSize, efiMemmap, &mapKey, &entrySize, &descVersion);
		if(lastStatus == EFI_SUCCESS) {
			allocSize = prevAllocSize;
			break;
		}

		if(lastStatus == EFI_BUFFER_TOO_SMALL) {
			gSystem->BootServices->FreePages((EFI_PHYSICAL_ADDRESS)efiMemmap, prevAllocSize / 0x1000);
			memmapSize += 2 * entrySize;
			prevAllocSize = memmapSize;
			lastStatus = gSystem->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, (prevAllocSize + 0xFFF) / 0x1000, (EFI_PHYSICAL_ADDRESS*)efiMemmap);
			if(EFI_ERROR(lastStatus)) return nullptr;
		}
	}

	efiMemmapEntryCount = memmapSize / entrySize;

	// Calculate total physical memory installed
	size_t totalMemoryPages = 0;
	for(size_t i = 0; i < efiMemmapEntryCount; i++)
	{
		EFI_MEMORY_DESCRIPTOR* desc = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>((uint8_t*)efiMemmap + i * entrySize);

		switch(desc->Type)
		{
			case EfiLoaderCode:
			case EfiLoaderData:
			case EfiBootServicesCode:
			case EfiBootServicesData:
			case EfiRuntimeServicesCode:
			case EfiRuntimeServicesData:
			case EfiConventionalMemory:
			case EfiACPIMemoryNVS:
			case EfiUnusableMemory:
			case EfiReservedMemoryType:
			case EfiMemoryMappedIO:
			case EfiMemoryMappedIOPortSpace:
			case EfiACPIReclaimMemory: totalMemoryPages += desc->NumberOfPages; break;
			default: break;
		}
	}

	// Get page count needed for the page tables
	size_t ptCount = (totalMemoryPages + 511) / 512;
	size_t pdCount = (ptCount + 511) / 512;
	size_t pdptCount = (pdCount + 511) / 512;
	size_t pml4Count = (pdptCount + 511) / 512;
	size_t PageTableReservePages = ptCount + pdCount + pdptCount + pml4Count;

	// Get page count needed for the physical bitmap allocator
	size_t BytesNeededForPhysAlloc = (totalMemoryPages + 7) / 8; // 1 bit / page
	size_t PagesNeededForPhysAlloc = (BytesNeededForPhysAlloc + 0xFFF) / 0x1000;

	const size_t SysTablePages = PAGE_ALIGN_UP(sizeof(SystemTable)) / 0x1000;
	const size_t bufSize = efiMemmapEntryCount * sizeof(MemoryRegion);
	const size_t bufPages = (bufSize + 0xFFF) / 0x1000;

	// Total buffer size:
	// Pages needed for page tables + Guard page
	// Pages needed for the physical bitmap allocator + Guard page
	// 32KiB kernel bootstrap stack + Guard page
	// Pages needed to store system table + guard page
	// Pages needed to store custom memory map + guard page
	// Pages needed for the SysKrnl64
	const size_t totalBufferSize = PageTableReservePages + 1 + PagesNeededForPhysAlloc + 1 + 8 + 1 + SysTablePages + 1 + bufPages + 1 + SysKrnl64PageCount;

	// Allocate another buffer with space enough for all of our regions for our own memory table

	size_t regionOffset = 0;

	for(size_t i = 0; i < efiMemmapEntryCount; i++)
	{
		EFI_MEMORY_DESCRIPTOR* desc = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>((uint8_t*)efiMemmap + i * entrySize);

		if(desc->Type == EfiConventionalMemory && desc->NumberOfPages >= totalBufferSize) {
			regionStart = desc->PhysicalStart;
			lastStatus = gSystem->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, totalBufferSize, &regionStart);
			if(EFI_ERROR(lastStatus)) return nullptr;
			break;
		}
	}

	memset((void*)regionStart, 0, totalBufferSize * 0x1000);

	static uintptr_t GuardPages[5];

	uint64_t* PageTablesAddr = reinterpret_cast<uint64_t*>(regionStart);
	PageTablesPhys = reinterpret_cast<uintptr_t>(PageTablesAddr);
	regionOffset += PageTableReservePages;
	GuardPages[0] = regionOffset++;
	uintptr_t PhysAllocAddr = regionStart + regionOffset * 0x1000;
	regionOffset += PagesNeededForPhysAlloc;
	GuardPages[1] = regionOffset++;
	uintptr_t StackAddr = regionStart + regionOffset * 0x1000;
	regionOffset += 8;
	GuardPages[2] = regionOffset++;
	SystemTable* System = reinterpret_cast<SystemTable*>(regionStart + regionOffset * 0x1000);
	regionOffset += SysTablePages;
	GuardPages[3] = regionOffset++;
	MemoryRegion* regions = reinterpret_cast<MemoryRegion*>(regionStart + regionOffset * 0x1000);
	regionOffset += bufPages;
	GuardPages[4] = regionOffset++;
	uintptr_t SysKrnl64LoadAddr = regionStart + regionOffset * 0x1000;
	regionOffset += SysKrnl64PageCount;

	// Requery the UEFI Memory map to reflect the latest AllocatePages modification

	if(EFI_ERROR((lastStatus = gSystem->BootServices->FreePages(reinterpret_cast<uintptr_t>(efiMemmap), (prevAllocSize + 0xFFF) / 0x1000)))) {
		printf("[BOOTMGR] [SysTableBuilder] [ERROR]: Failed to free old Memmap buffer: 0x%llX\r\n", lastStatus);
		return nullptr;
	}

	efiMemmap = nullptr;
	memmapSize = 0, mapKey = 0;
	prevAllocSize = 0;
	descVersion = 0;
	
	lastStatus = gSystem->BootServices->GetMemoryMap(&memmapSize, efiMemmap, &mapKey, &entrySize, &descVersion);
	if(lastStatus != EFI_BUFFER_TOO_SMALL) return nullptr;

	memmapSize += 2 * entrySize;
	efiMemmapAddr = 0;
	lastStatus = gSystem->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, (memmapSize + 0xFFF) / 0x1000, &efiMemmapAddr);
	prevAllocSize = memmapSize;
	if(EFI_ERROR(lastStatus)) return nullptr;
	efiMemmap = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>(efiMemmapAddr);

	while(true) {
		lastStatus = gSystem->BootServices->GetMemoryMap(&memmapSize, efiMemmap, &mapKey, &entrySize, &descVersion);
		if(lastStatus == EFI_SUCCESS) {
			allocSize = prevAllocSize;
			break;
		}

		if(lastStatus == EFI_BUFFER_TOO_SMALL) {
			gSystem->BootServices->FreePages((EFI_PHYSICAL_ADDRESS)efiMemmap, prevAllocSize / 0x1000);
			memmapSize += 2 * entrySize;
			prevAllocSize = memmapSize;
			lastStatus = gSystem->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, (prevAllocSize + 0xFFF) / 0x1000, (EFI_PHYSICAL_ADDRESS*)efiMemmap);
			if(EFI_ERROR(lastStatus)) return nullptr;
		}
	}

	efiMemmapEntryCount = memmapSize / entrySize;
	
	// Now that we have the buffer, we fill it out.

	MemoryRegion* lastRegion = nullptr;
	size_t regionCount = 0;
	for(size_t i = 0; i < efiMemmapEntryCount; i++)
	{
		EFI_MEMORY_DESCRIPTOR* desc = reinterpret_cast<EFI_MEMORY_DESCRIPTOR*>((uint8_t*)efiMemmap + i * entrySize);

		MemoryRegionType convType = {};
		switch(desc->Type) {
			case EfiReservedMemoryType:
			case EfiMemoryMappedIO:
			case EfiMemoryMappedIOPortSpace:
			case EfiPalCode: convType = MEMTYPE_HARDWARE_RESERVED; break;
			case EfiLoaderCode:
			case EfiLoaderData:
			case EfiBootServicesCode:
			case EfiBootServicesData:
			case EfiRuntimeServicesCode:
			case EfiRuntimeServicesData:
			case EfiConventionalMemory: convType = MEMTYPE_USABLE; break;
			case EfiUnusableMemory:
			case EfiPersistentMemory: convType = MEMTYPE_BAD_MEMORY; break;
			case EfiACPIReclaimMemory: convType = MEMTYPE_ACPIRECLAIM; break;
			case EfiACPIMemoryNVS: convType = MEMTYPE_ACPINVS; break;
			default: convType = MEMTYPE_BAD_MEMORY; break;
		}

		// Check if region overlaps the software reserved region
		bool remapped = false;
		uintptr_t descStart = desc->PhysicalStart;
		uintptr_t descEnd = descStart + desc->NumberOfPages * 0x1000;
		uintptr_t allocEnd = regionStart + totalBufferSize * 0x1000;
		if(desc->Type == EfiLoaderData && descStart < allocEnd && regionStart < descEnd && !remapped) {
			if(regionStart <= descStart && allocEnd >= descEnd) {
				convType = MEMTYPE_SOFTWARE_RESERVED;
				remapped = true;
			}
			else if(regionStart <= descStart) {
				descStart = allocEnd;
				MemoryRegion* newRegion = &regions[regionCount++];
				newRegion->phys = regionStart;
				newRegion->length = totalBufferSize * 0x1000;
				newRegion->type = MEMTYPE_SOFTWARE_RESERVED;
				newRegion = &regions[regionCount++];
				newRegion->phys = descStart;
				newRegion->length = descEnd - descStart;
				newRegion->type = convType;

				remapped = true;
				lastRegion = newRegion;

				continue;
			} else if(allocEnd >= descEnd) {
				descEnd = regionStart;
				MemoryRegion* newRegion = &regions[regionCount++];
				newRegion->phys = descStart;
				newRegion->length = descEnd - descStart;
				newRegion->type = convType;
				newRegion = &regions[regionCount++];
				newRegion->phys = regionStart;
				newRegion->length = totalBufferSize * 0x1000;
				newRegion->type = MEMTYPE_SOFTWARE_RESERVED;

				remapped = true;
				lastRegion = nullptr;
				continue;
			} else {
				MemoryRegion* newRegion = &regions[regionCount++];

				newRegion->phys = descStart;
				newRegion->length = regionStart - descStart;
				newRegion->type = convType;

				newRegion = &regions[regionCount++];

				newRegion->phys = regionStart;
				newRegion->length = totalBufferSize * 0x1000;
				newRegion->type = MEMTYPE_SOFTWARE_RESERVED;

				newRegion = &regions[regionCount++];
				newRegion->phys = allocEnd;
				newRegion->length = descEnd - allocEnd;
				newRegion->type = convType;

				remapped = true;
				lastRegion = newRegion;
				continue;
			}
		}

		if(lastRegion && lastRegion->type == convType && lastRegion->phys + lastRegion->length == desc->PhysicalStart) {
			lastRegion->length += desc->NumberOfPages * PAGE_SIZE; // Merge same converted type entries that are side-by-side
			continue;
		} else {
			// Create new entry
			MemoryRegion* newRegion = &regions[regionCount++];
			newRegion->length = desc->NumberOfPages * PAGE_SIZE;
			newRegion->phys = desc->PhysicalStart;
			newRegion->type = convType;
			lastRegion = newRegion;
		}
	}

	// Query the GOP Framebuffer
	gop = nullptr;
	lastStatus = gSystem->BootServices->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, nullptr, (void**)&gop);
	if(EFI_ERROR(lastStatus)) return nullptr;
	if(!gop) {
		lastStatus = EFI_DEVICE_ERROR;
		return nullptr;
	}

	// Fill out the system table

	System->memTable.regions = reinterpret_cast<MemoryRegion*>(reinterpret_cast<uintptr_t>(regions) - regionStart + MapAddr);
	System->memTable.regionCount = regionCount;
	System->fb.fbBase = reinterpret_cast<uintptr_t>(gop) - regionStart + MapAddr;

	
	// Select the highest preferred supported resolution

	VideoResolution preferredResolutions[] = {
		{1920, 1080, PIXEL_FORMAT_R8G8B8A8, 0, 0},
		{1920, 1080, PIXEL_FORMAT_B8G8R8A8, 0, 0},
		{1600, 900, PIXEL_FORMAT_R8G8B8A8, 0, 0},
		{1600, 900, PIXEL_FORMAT_B8G8R8A8, 0, 0},
		{1280, 720, PIXEL_FORMAT_R8G8B8A8, 0, 0},
		{1280, 720, PIXEL_FORMAT_B8G8R8A8, 0, 0},
	};
	size_t pResCount = sizeof(preferredResolutions) / sizeof(preferredResolutions[0]);
	
	EFI_GRAPHICS_OUTPUT_PROTOCOL* Gop = reinterpret_cast<EFI_GRAPHICS_OUTPUT_PROTOCOL*>(gop);

	for(size_t i = 0; i < Gop->Mode->MaxMode; i++)
	{
		EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info;
		size_t infoSize;

		lastStatus = Gop->QueryMode(Gop, i, &infoSize, &info);
		if(EFI_ERROR(lastStatus)) continue;

		for(size_t j = 0; j < pResCount; j++)
		{
			const VideoResolution pRes = preferredResolutions[j];
			if(pRes.resWidth == info->HorizontalResolution && pRes.resHeight == info->VerticalResolution && info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
				System->fb.currentResolution = pRes;
				System->fb.currentResolution.gopIndex = i;
				System->fb.currentResolution.resPitch = info->PixelsPerScanLine * 4; // Convert to pixels/scanline to bytes/scanline(32bpp)
			}
		}
	}

	// Query the ACPI RSDP

	EFI_CONFIGURATION_TABLE* cfgTable = gSystem->ConfigurationTable;
	
	for(size_t i = 0; i < gSystem->NumberOfTableEntries; i++)
	{
		EFI_GUID guid = cfgTable[i].VendorGuid;
		if(CompareGUID(guid, gEfiAcpi20TableGuid)) {
			System->ACPI_RSDP = (uintptr_t)cfgTable[i].VendorTable;
			break;
		}
	}

	// Query the SMBIOS 3.x, if not found -> SMBIOS 2.x

	for(size_t i = 0; i < gSystem->NumberOfTableEntries; i++)
	{
		EFI_GUID guid = cfgTable[i].VendorGuid;
		if(CompareGUID(guid, gEfiSmbios3TableGuid)) {
			System->SMBIOS_PTR = (uintptr_t)cfgTable[i].VendorTable;
			System->SMBIOS_VersionMajor = 3;
			SMBIOS_TABLE_3_0_ENTRY_POINT* ep = (SMBIOS_TABLE_3_0_ENTRY_POINT*)cfgTable[i].VendorTable;
			System->SMBIOS_VersionMinor = ep->MinorVersion;
			break;
		}
	}

	for(size_t i = 0; i < gSystem->NumberOfTableEntries; i++)
	{
		EFI_GUID guid = cfgTable[i].VendorGuid;
		if(CompareGUID(guid, gEfiSmbiosTableGuid)) {
			System->SMBIOS_PTR = (uintptr_t)cfgTable[i].VendorTable;
			System->SMBIOS_VersionMajor = 2;
			SMBIOS_TABLE_ENTRY_POINT* ep = (SMBIOS_TABLE_ENTRY_POINT*)cfgTable[i].VendorTable;
			System->SMBIOS_VersionMinor = ep->MinorVersion;
			break;
		}
	}

	uint64_t* PageTables = reinterpret_cast<uint64_t*>(PageTablesAddr);
	// Setup page tables to map out the entire Software Reserved(kernel) region

	// Setup the free list for O(1) access of the page tables

	struct PageTableNode {
		PageTableNode* next;
	};

	PageTableNode* freeListHead = reinterpret_cast<PageTableNode*>(PageTables + 512);
	PageTableNode* node = freeListHead;

	for(size_t i = 1; i < PageTableReservePages; i++) {
		node->next = reinterpret_cast<PageTableNode*>(PageTables + i * 512);
		node = node->next;
	}

	node->next = nullptr;

	auto allocate_page_table = [&]() -> uint64_t* {
		PageTableNode* page = freeListHead;
		freeListHead = freeListHead->next;
		memset(page, 0, 0x1000);
		return reinterpret_cast<uint64_t*>(page);
	};
	
	// Setup a lambda for cleaner code

	auto map_page = [&](uint64_t* PML4, uintptr_t phys, uintptr_t virt) {
		const uint16_t pml4_idx = (virt >> 39) & 0x1FF;
		const uint16_t pdpt_idx = (virt >> 30) & 0x1FF;
		const uint16_t pd_idx = (virt >> 21) & 0x1FF;
		const uint16_t pt_idx = (virt >> 12) & 0x1FF;

		uint64_t* pdpt_entry = &PML4[pml4_idx];
		if(!(*pdpt_entry & PTE_PRESENT)) {
			uint64_t* new_pdpt = allocate_page_table();
			*pdpt_entry = MAKE_PTE(reinterpret_cast<uintptr_t>(new_pdpt), PTE_PRESENT | PTE_RW);
		}
		uint64_t* pdpt = reinterpret_cast<uint64_t*>(*pdpt_entry & PTE_PHYS_MASK);

		uint64_t* pd_entry = &pdpt[pdpt_idx];
		if(!(*pd_entry & PTE_PRESENT)) {
			uint64_t* new_pd = allocate_page_table();
			*pd_entry = MAKE_PTE(reinterpret_cast<uintptr_t>(new_pd), PTE_PRESENT | PTE_RW);
		}

		uint64_t* pd = reinterpret_cast<uint64_t*>(*pd_entry & PTE_PHYS_MASK);

		uint64_t* pt_entry = &pd[pd_idx];
		if(!(*pt_entry & PTE_PRESENT)) {
			uint64_t* new_pt = allocate_page_table();
			*pt_entry = MAKE_PTE(reinterpret_cast<uintptr_t>(new_pt), PTE_PRESENT | PTE_RW);
		}

		uint64_t* pt = reinterpret_cast<uint64_t*>(*pt_entry & PTE_PHYS_MASK);

		pt[pt_idx] = MAKE_PTE(phys, PTE_PRESENT | PTE_RW);
	};

	// Map entire kernel memory region starting from 0xFFFF800000000000

	for(size_t i = 0; i < totalBufferSize; i++) {
		for(size_t j = 0; j < sizeof(GuardPages) / sizeof(GuardPages[0]); j++) if((MapAddr + i * 0x1000) - MapAddr == GuardPages[j] * 0x1000) continue;
		map_page(PageTables, regionStart + i * 0x1000, MapAddr + i * 0x1000);
	}

	// Map an extra page for the CR3 switch

	map_page(PageTables, StackAddr, StackAddr);

	// Store memory layouts

	System->memLayout.PageTableAddr = reinterpret_cast<uintptr_t>(PageTables) - regionStart + MapAddr;
	System->memLayout.PageTablePageCount = PageTableReservePages;
	System->memLayout.PhysAllocBitmapAddr = PhysAllocAddr - regionStart + MapAddr;
	System->memLayout.PhysAllocBitmapPages = PagesNeededForPhysAlloc;
	System->memLayout.StackAddr = StackAddr - regionStart + MapAddr;
	System->memLayout.StackPageCount = 8;
	System->memLayout.SysKrnl64PhysAddr = SysKrnl64LoadAddr;
	System->memLayout.SysKrnl64LoadSize = SysKrnl64PageCount * 0x1000;
	System->memLayout.KrnlMemRegionSize = regionOffset * 0x1000;
	System->memLayout.regionStartPhys = regionStart;
	System->memLayout.NextPageTableFreePtr = reinterpret_cast<uintptr_t>(freeListHead);
	System->usableRAMPages = totalMemoryPages;

	// Store kernel structure memory regions

	// Page Tables
	System->kernelStructureRegions[0].phys = reinterpret_cast<uintptr_t>(PageTables);
	System->kernelStructureRegions[0].virt = System->memLayout.PageTableAddr;
	System->kernelStructureRegions[0].totalPages = PageTableReservePages;
	System->kernelStructureRegions[0].guardPage = true;
	System->kernelStructureRegions[0].writeProtected = false;
	System->kernelStructureRegions[0].execProtected = true;

	// Physical Allocator Bitmap
	System->kernelStructureRegions[1].phys = PhysAllocAddr;
	System->kernelStructureRegions[1].virt = System->memLayout.PhysAllocBitmapAddr;
	System->kernelStructureRegions[1].totalPages = System->memLayout.PhysAllocBitmapPages;
	System->kernelStructureRegions[1].guardPage = true;
	System->kernelStructureRegions[1].writeProtected = false;
	System->kernelStructureRegions[1].execProtected = true;

	// 32KiB Bootstrap stack
	System->kernelStructureRegions[2].phys = StackAddr;
	System->kernelStructureRegions[2].virt = System->memLayout.StackAddr;
	System->kernelStructureRegions[2].totalPages = System->memLayout.StackPageCount;
	System->kernelStructureRegions[2].guardPage = true;
	System->kernelStructureRegions[2].writeProtected = false;
	System->kernelStructureRegions[2].execProtected = false;

	// System table
	System->kernelStructureRegions[3].phys = reinterpret_cast<uintptr_t>(System);
	System->kernelStructureRegions[3].virt = reinterpret_cast<uintptr_t>(System) - regionStart + MapAddr;
	System->kernelStructureRegions[3].totalPages = SysTablePages;
	System->kernelStructureRegions[3].guardPage = true;
	System->kernelStructureRegions[3].writeProtected = true;
	System->kernelStructureRegions[3].execProtected = true;

	// Custom memory map
	System->kernelStructureRegions[4].phys = reinterpret_cast<uintptr_t>(regions);
	System->kernelStructureRegions[4].virt = reinterpret_cast<uintptr_t>(System->memTable.regions);
	System->kernelStructureRegions[4].totalPages = bufPages;
	System->kernelStructureRegions[4].guardPage = true;
	System->kernelStructureRegions[4].writeProtected = true;
	System->kernelStructureRegions[4].execProtected = true;

	// SysKrnl64 PT_LOAD segments
	System->kernelStructureRegions[5].phys = reinterpret_cast<uintptr_t>(SysKrnl64LoadAddr);
	System->kernelStructureRegions[5].virt = reinterpret_cast<uintptr_t>(SysKrnl64LoadAddr) - regionStart + MapAddr;
	System->kernelStructureRegions[5].totalPages = SysKrnl64PageCount;
	System->kernelStructureRegions[5].guardPage = false;
	System->kernelStructureRegions[5].writeProtected = false;
	System->kernelStructureRegions[5].execProtected = false;

	
	lastStatus = EFI_SUCCESS;
	return System;
}