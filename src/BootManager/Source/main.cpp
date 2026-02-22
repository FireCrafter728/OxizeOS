EFI_SYSTEM_TABLE* BootMgr::gSystem = nullptr;

extern "C" EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* System)
{
	BootMgr::gSystem = System;

	clrscr();

	EnableSSE();

	__cxa_init_global_ctors();

	BootMgr::SysTable::SysTable sysTableBuilder;

	BootMgr::FS::FS fs;

	EFI_HANDLE VolumeHandle = fs.GetVolumeHandle(ImageHandle);
	if(!VolumeHandle) {
		printf("[BOOTMGR] [ERROR]: Failed to open the ESP Handle: 0x%llX\r\n", fs.GetLastStatus());
		HaltSystem();
	}

	EFI_FILE_PROTOCOL* ESP = fs.OpenVolume(VolumeHandle);
	if(!ESP) {
		System->ConOut->OutputString(System->ConOut, (CHAR16*)L"Failed to open the ESP\r\n");
		HaltSystem();
	}

	EFI_FILE_PROTOCOL* TskSchl = fs.OpenFile(ESP, L"\\EFI\\OxizeOS\\tskschl.exe");
	if(!TskSchl) {
		System->ConOut->OutputString(System->ConOut, (CHAR16*)L"Failed to find \\EFI\\OxizeOS\\tskschl.exe\r\n");
		HaltSystem();
	}

	BootMgr::ELF::ELF elf;
	BootMgr::ELF::ELF_Handle elfHandle;
	elf.CreateHandle(&elfHandle, TskSchl, &fs);

	SystemTable* sysTable = sysTableBuilder.BuildSystemTable(elfHandle.LoadPages);
	if(!sysTable) {
		printf("[OXIZEOS-BOOTMGR] [ERROR]: Failed to build a System Table for the kernel: %llu\r\n", sysTableBuilder.GetLastStatus());
		HaltSystem();
	}
	elf.SetLoadAddr(&elfHandle, sysTable->memLayout.TskSchlPhysAddr);
	elf.SetVirtLoadAddr(&elfHandle, sysTable->memLayout.TskSchlPhysAddr - sysTableBuilder.getRegionStartAddr() + BootMgr::MapAddr);

	printf("Loading to: 0x%llX\r\n", sysTable->memLayout.TskSchlPhysAddr);

	elf.LoadImage(&elfHandle);

	// Set the preferred GOP Framebuffer mode

	EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = reinterpret_cast<EFI_GRAPHICS_OUTPUT_PROTOCOL*>(sysTableBuilder.getEfiGOP());
	gop->SetMode(gop, sysTable->fb.currentResolution.gopIndex);
	gop = nullptr;
	if(EFI_ERROR(System->BootServices->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, nullptr, (void**)&gop))) {
		printf("[BOOTMGR] [ERROR]: Failed to query new GOP Interface\r\n");
		HaltSystem();
	}
	sysTable->fb.fbBase = gop->Mode->FrameBufferBase;

	uint32_t color = 0xFF000000;
	uint32_t* fb = reinterpret_cast<uint32_t*>(sysTable->fb.fbBase);
	for(size_t x = 0; x < sysTable->fb.currentResolution.resWidth; x++)
		for(size_t y = 0; y < sysTable->fb.currentResolution.resHeight; y++)
			fb[y * sysTable->fb.currentResolution.resPitch + x] = color;
	
	// Copy ExecuteKernel function to the start of the kernel data area
	// mapped by both UEFI & our own page tables and execute it

	uintptr_t krnlExecLoadAddr = sysTable->memLayout.DataAreaAddr - BootMgr::MapAddr + sysTableBuilder.getRegionStartAddr();
	size_t krnlExecLoadSize = (uintptr_t)ExecuteKernelEnd - (uintptr_t)ExecuteKernel;
	memcpy((void*)krnlExecLoadAddr, (void*)ExecuteKernel, krnlExecLoadSize);

	typedef void (*KrnlExec)(uintptr_t Entry, SystemTable* systemTable, uint64_t CR3, uintptr_t StackAddr);
	KrnlExec krnl = (KrnlExec)krnlExecLoadAddr;
	printf("krnlAddr: 0x%llX\r\n", krnl);
	printf("Region start: 0x%llX\r\n", sysTableBuilder.getRegionStartAddr());
	uintptr_t tskschlVirt = (elfHandle.LoadAddr + elfHandle.header.EntryOffset - elfHandle.extraOffset) - sysTableBuilder.getRegionStartAddr() + BootMgr::MapAddr;

	uint16_t pml4_idx = (tskschlVirt >> 39) & 0x1FF;
	uint16_t pdpt_idx = (tskschlVirt >> 30) & 0x1FF;
	uint16_t pd_idx   = (tskschlVirt >> 21) & 0x1FF;
	uint16_t pt_idx   = (tskschlVirt >> 12) & 0x1FF;

	uint64_t* pml4 = reinterpret_cast<uint64_t*>(sysTable->memLayout.PageTableAddr - BootMgr::MapAddr + sysTableBuilder.getRegionStartAddr());

	uint64_t pml4_entry = pml4[pml4_idx];
	if (!(pml4_entry & BootMgr::PTE_PRESENT)) {
	    printf("[BOOTMGR] [ERROR]: PML4 entry not present!\r\n");
	    HaltSystem();
	}

	uint64_t* pdpt = reinterpret_cast<uint64_t*>(pml4_entry & BootMgr::PTE_PHYS_MASK);
	uint64_t pdpt_entry = pdpt[pdpt_idx];
	if (!(pdpt_entry & BootMgr::PTE_PRESENT)) {
	    printf("[BOOTMGR] [ERROR]: PDPT entry not present!\r\n");
	    HaltSystem();
	}

	uint64_t* pd = reinterpret_cast<uint64_t*>(pdpt_entry & BootMgr::PTE_PHYS_MASK);
	uint64_t pd_entry = pd[pd_idx];
	if (!(pd_entry & BootMgr::PTE_PRESENT)) {
	    printf("[BOOTMGR] [ERROR]: PD entry not present!\r\n");
	    HaltSystem();
	}

	uint64_t* pt = reinterpret_cast<uint64_t*>(pd_entry & BootMgr::PTE_PHYS_MASK);
	uint64_t pt_entry = pt[pt_idx];
	if (!(pt_entry & BootMgr::PTE_PRESENT)) {
	    printf("[BOOTMGR] [ERROR]: PT entry not present!\r\n");
	    HaltSystem();
	}

	uintptr_t physAddr = pt_entry & BootMgr::PTE_PHYS_MASK;
	printf("Kernel entry VA 0x%llX is mapped to PA 0x%llX\r\n", tskschlVirt, physAddr);

	krnl(tskschlVirt, reinterpret_cast<SystemTable*>(reinterpret_cast<uintptr_t>(sysTable) - sysTableBuilder.getRegionStartAddr() + BootMgr::MapAddr), BootMgr::MAKE_CR3(sysTableBuilder.getPageTablesPhysAddr(), 0), sysTable->memLayout.StackAddr + sysTable->memLayout.StackPageCount * 0x1000);
	
	return EFI_DEVICE_ERROR;
} 