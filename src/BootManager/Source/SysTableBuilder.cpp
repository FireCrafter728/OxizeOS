#include <SysTableBuilder.hpp>

using namespace BootMgr::SysTable;

SystemTable* SysTable::BuildSystemTable(size_t TskSchlPageCount, EFI_SYSTEM_TABLE* gSystem)
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
			case EfiACPIReclaimMemory: totalMemoryPages += desc->NumberOfPages; break;
			default: break;
		}
	}

	printf("[BOOTMGR] [SysTableBuilder] [INFO]: Total mememory installed pages: 0x%llX, bytes: 0x%llX\r\n", totalMemoryPages, totalMemoryPages * 0x1000);

	size_t ptCount = (totalMemoryPages + 511) / 512;
	size_t pdCount = (ptCount + 511) / 512;
	size_t pdptCount = (pdCount + 511) / 512;
	size_t pml4Count = (pdptCount + 511) / 512;
	size_t PageTableReservePages = ptCount + pdCount + pdptCount + pml4Count;

	const size_t SysTablePages = PAGE_ALIGN_UP(sizeof(SystemTable)) / 0x1000;
	const size_t bufSize = efiMemmapEntryCount * sizeof(MemoryRegion);
	const size_t bufPages = (bufSize + 0xFFF) / 0x1000;
	// Total buffer size:
	// Pages needed for page tables + Guard page
	// 512KB Stack + Guard page
	// Pages needed to store system table + guard page
	// Pages needed to store custom memory map + guard page
	// 64MiB Kernel data area + guard page
	// Pages needed for the task scheduler
	const size_t totalBufferSize = PageTableReservePages + 1 + 128 + 1 + SysTablePages + 1 + bufPages + 1 + 0x4000 + 1 + TskSchlPageCount;

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

	uint64_t* PageTables = reinterpret_cast<uint64_t*>(regionStart);
	PageTablesPhys = reinterpret_cast<uintptr_t>(PageTables);
	regionOffset += PageTableReservePages;
	GuardPages[0] = regionOffset++;
	uintptr_t StackAddr = regionStart + regionOffset * 0x1000;
	regionOffset += 128;
	GuardPages[1] = regionOffset++;
	SystemTable* System = reinterpret_cast<SystemTable*>(regionStart + regionOffset * 0x1000);
	regionOffset += SysTablePages;
	GuardPages[2] = regionOffset++;
	MemoryRegion* regions = reinterpret_cast<MemoryRegion*>(regionStart + regionOffset * 0x1000);
	regionOffset += bufPages;
	GuardPages[3] = regionOffset++;
	uintptr_t DataRegionAddr = regionStart + regionOffset * 0x1000;
	regionOffset += 0x4000;
	GuardPages[4] = regionOffset++;
	uintptr_t TskSchlLoadAddr = regionStart + regionOffset * 0x1000;
	regionOffset += TskSchlPageCount;
	printf("loadAddr: 0x%llX\r\n", TskSchlLoadAddr - regionStart);

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
				printf("remapping, case 1\r\n");
				convType = MEMTYPE_SOFTWARE_RESERVED;
				remapped = true;
			}
			else if(regionStart <= descStart) {
				printf("remapping, case 2\r\n");
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
				printf("remapping, case 3\r\n");
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
				printf("remapping, case 4\r\n");
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

	for(size_t i = 0; i < regionCount; i++)
	{
		MemoryRegion region = regions[i];
		printf("Memory region %llu: start=0x%llX, length=0x%llX, type=%llu\r\n", i, region.phys, region.length, region.type);
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

	System->memTable.regions = regions;
	System->memTable.regionCount = regionCount;
	System->fb.fbBase = reinterpret_cast<uintptr_t>(gop) - regionStart + MapAddr;

	
	// Select the highest preferred supported resolution

	VideoResolution preferredResolutions[] = {
		{1920, 1080, PIXEL_FORMAT_R8G8B8A8},
		{1920, 1080, PIXEL_FORMAT_B8G8R8A8},
		{1600, 900, PIXEL_FORMAT_R8G8B8A8},
		{1600, 900, PIXEL_FORMAT_B8G8R8A8},
		{1280, 720, PIXEL_FORMAT_R8G8B8A8},
		{1280, 720, PIXEL_FORMAT_B8G8R8A8},
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
				System->fb.currentResolution = preferredResolutions[j];
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

	uintptr_t OffsetInPageTables = 0x1000;
	// Setup page tables to map out the entire Software Reserved(kernel) region
	for(uintptr_t Addr = MapAddr; Addr < MapAddr + regionOffset * 0x1000; Addr += 0x1000)
	{
		for(size_t i = 0; i < sizeof(GuardPages) / sizeof(GuardPages[0]); i++) if(Addr - MapAddr == GuardPages[i] * 0x1000) continue;
		uint16_t pml4_idx = (Addr >> 39) & 0x1FF;
		uint16_t pdpt_idx = (Addr >> 30) & 0x1FF;
		uint16_t pd_idx = (Addr >> 21) & 0x1FF;
		uint16_t pt_idx = (Addr >> 12) & 0x1FF;

		// Get PDPT
		uint64_t* pml4 = PageTables;
		uint64_t* pdpt_entry = &pml4[pml4_idx];
		if(!(*pdpt_entry & PTE_PRESENT)) {
			// Entry doesn't exist, create it
			*pdpt_entry = MAKE_PTE(reinterpret_cast<uintptr_t>(PageTables) + OffsetInPageTables, PTE_PRESENT | PTE_RW);
			OffsetInPageTables += 0x1000;
			memset((void*)(*pdpt_entry & PTE_PHYS_MASK), 0, 0x1000);
		}
		uint64_t* pdpt = reinterpret_cast<uint64_t*>(*pdpt_entry & PTE_PHYS_MASK);

		// Get PD
		uint64_t* pd_entry = &pdpt[pdpt_idx];
		if(!(*pd_entry & PTE_PRESENT)) {
			*pd_entry = MAKE_PTE(reinterpret_cast<uintptr_t>(PageTables) + OffsetInPageTables, PTE_PRESENT | PTE_RW);
			OffsetInPageTables += 0x1000;
			memset((void*)(*pd_entry & PTE_PHYS_MASK), 0, 0x1000);
		}
		uint64_t* pd = reinterpret_cast<uint64_t*>(*pd_entry & PTE_PHYS_MASK);

		// Get PT
		uint64_t* pt_entry = &pd[pd_idx];
		if(!(*pt_entry & PTE_PRESENT)) {
			*pt_entry = MAKE_PTE(reinterpret_cast<uintptr_t>(PageTables) + OffsetInPageTables, PTE_PRESENT | PTE_RW);
			OffsetInPageTables += 0x1000;
			memset((void*)(*pt_entry & PTE_PHYS_MASK), 0, 0x1000);
		}
		uint64_t* pt = reinterpret_cast<uint64_t*>(*pt_entry & PTE_PHYS_MASK);

		// Create page
		uint64_t* page_entry = &pt[pt_idx];
		*page_entry = MAKE_PTE(regionStart + (Addr - MapAddr), PTE_PRESENT | PTE_RW);
	}

	// Create one extra entry to identity-map first page of data area to be able to execute the task scheduler

	uint16_t pml4_idx = (DataRegionAddr >> 39) & 0x1FF;
	uint16_t pdpt_idx = (DataRegionAddr >> 30) & 0x1FF;
	uint16_t pd_idx = (DataRegionAddr >> 21) & 0x1FF;
	uint16_t pt_idx = (DataRegionAddr >> 12) & 0x1FF;
	// Get PDPT
	uint64_t* pml4 = PageTables;
	uint64_t* pdpt_entry = &pml4[pml4_idx];
	if(!(*pdpt_entry & PTE_PRESENT)) {
		// Entry doesn't exist, create it
		*pdpt_entry = MAKE_PTE(reinterpret_cast<uintptr_t>(PageTables) + OffsetInPageTables, PTE_PRESENT | PTE_RW);
		OffsetInPageTables += 0x1000;
		memset((void*)(*pdpt_entry & PTE_PHYS_MASK), 0, 0x1000);
	}
	uint64_t* pdpt = reinterpret_cast<uint64_t*>(*pdpt_entry & PTE_PHYS_MASK);
	// Get PD
	uint64_t* pd_entry = &pdpt[pdpt_idx];
	if(!(*pd_entry & PTE_PRESENT)) {
		*pd_entry = MAKE_PTE(reinterpret_cast<uintptr_t>(PageTables) + OffsetInPageTables, PTE_PRESENT | PTE_RW);
		OffsetInPageTables += 0x1000;
		memset((void*)(*pd_entry & PTE_PHYS_MASK), 0, 0x1000);
	}
	uint64_t* pd = reinterpret_cast<uint64_t*>(*pd_entry & PTE_PHYS_MASK);
	// Get PT
	uint64_t* pt_entry = &pd[pd_idx];
	if(!(*pt_entry & PTE_PRESENT)) {
		*pt_entry = MAKE_PTE(reinterpret_cast<uintptr_t>(PageTables) + OffsetInPageTables, PTE_PRESENT | PTE_RW);
		OffsetInPageTables += 0x1000;
		memset((void*)(*pt_entry & PTE_PHYS_MASK), 0, 0x1000);
	}
	uint64_t* pt = reinterpret_cast<uint64_t*>(*pt_entry & PTE_PHYS_MASK);
	// Create page
	uint64_t* page_entry = &pt[pt_idx];
	*page_entry = MAKE_PTE(DataRegionAddr, PTE_PRESENT | PTE_RW);

	System->memLayout.PageTableAddr = reinterpret_cast<uintptr_t>(PageTables) - regionStart + MapAddr;
	System->memLayout.PageTablePageCount = PageTableReservePages;
	System->memLayout.StackAddr = StackAddr - regionStart + MapAddr;
	System->memLayout.StackPageCount = 128;
	System->memLayout.DataAreaAddr = DataRegionAddr - regionStart + MapAddr;
	System->memLayout.DataAreaPageCount = 0x4000;
	System->memLayout.TskSchlPhysAddr = TskSchlLoadAddr;
	System->memLayout.TskSchlLoadSize = TskSchlPageCount * 0x1000;
	System->memLayout.KrnlMemRegionSize = regionOffset * 0x1000;
	System->memLayout.PageTableOffset = OffsetInPageTables;
	System->memLayout.regionStartPhys = regionStart;

	lastStatus = EFI_SUCCESS;
	return System;
}