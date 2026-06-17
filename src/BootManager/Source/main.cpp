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

	EFI_FILE_PROTOCOL* SysKrnl64 = fs.OpenFile(ESP, L"\\EFI\\OxizeOS\\syskrnl64.exe");
	if(!SysKrnl64) {
		System->ConOut->OutputString(System->ConOut, (CHAR16*)L"Failed to find \\EFI\\OxizeOS\\syskrnl64.exe\r\n");
		HaltSystem();
	}

	BootMgr::ELF::ELF elf;
	BootMgr::ELF::ELF_Handle elfHandle;
	elf.CreateHandle(&elfHandle, SysKrnl64, &fs);

	SystemTable* sysTable = sysTableBuilder.BuildSystemTable(elfHandle.LoadPages);
	if(!sysTable) {
		printf("[OXIZEOS-BOOTMGR] [ERROR]: Failed to build a System Table for the kernel: %llu\r\n", sysTableBuilder.GetLastStatus());
		HaltSystem();
	}
	elf.SetLoadAddr(&elfHandle, sysTable->memLayout.SysKrnl64PhysAddr);
	elf.SetVirtLoadAddr(&elfHandle, sysTable->memLayout.SysKrnl64PhysAddr - sysTableBuilder.getRegionStartAddr() + BootMgr::MapAddr);

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
	
	// Copy ExecuteKernel function to the start of the kernel bootstrap stack
	// mapped by both UEFI & our own page tables and execute it

	uintptr_t krnlExecLoadAddr = sysTable->memLayout.StackAddr - BootMgr::MapAddr + sysTableBuilder.getRegionStartAddr();
	size_t krnlExecLoadSize = (uintptr_t)ExecuteKernelEnd - (uintptr_t)ExecuteKernel;
	memcpy((void*)krnlExecLoadAddr, (void*)ExecuteKernel, krnlExecLoadSize);

	typedef void (*KrnlExec)(uintptr_t Entry, SystemTable* systemTable, uint64_t CR3, uintptr_t StackAddr);
	KrnlExec krnl = (KrnlExec)krnlExecLoadAddr;
	uintptr_t SysKrnl64Virt = (elfHandle.LoadAddr + elfHandle.header.EntryOffset - elfHandle.extraOffset) - sysTableBuilder.getRegionStartAddr() + BootMgr::MapAddr;

	krnl(SysKrnl64Virt, reinterpret_cast<SystemTable*>(reinterpret_cast<uintptr_t>(sysTable) - sysTableBuilder.getRegionStartAddr() + BootMgr::MapAddr), BootMgr::MAKE_CR3(sysTableBuilder.getPageTablesPhysAddr(), 0), sysTable->memLayout.StackAddr + sysTable->memLayout.StackPageCount * 0x1000);
	
	return EFI_DEVICE_ERROR;
}