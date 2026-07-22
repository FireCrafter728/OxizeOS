// SPDX-License-Identifier: GPL-3.0-or-later
//
// OxizeOS Operating System for the x86 amd64(x86_64) architecture
// Copyright (C) 2025-2026 FireCrafter728
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
	printf("SysKrnl64 phys addr: 0x%llX, SysKrnl64 virt addr: 0x%llX\r\n", sysTable->memLayout.SysKrnl64PhysAddr, sysTable->memLayout.SysKrnl64PhysAddr - sysTableBuilder.getRegionStartAddr() + BootMgr::MapAddr);
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

	// Call UEFI ExitBootServices
	// For that we need the most recent UEFI Memory map key

	UINTN memmapSize = 0, mapKey = 0, descriptorSize = 0;
	UINT32 descriptorVersion = 0;
	EFI_MEMORY_DESCRIPTOR* memmap = nullptr;
	EFI_STATUS status = System->BootServices->GetMemoryMap(&memmapSize, memmap, &mapKey, &descriptorSize, &descriptorVersion);
	if(status != EFI_BUFFER_TOO_SMALL)
	{
		printf("[BOOTMGR] [ERROR]: Failed to get the most recent UEFI memory map key, error code: 0x%llX\r\n", status);
		HaltSystem();
	}

	// We do not need to allocate the new buffer, as the most recent mapKey should be returned

	status = System->BootServices->ExitBootServices(ImageHandle, mapKey);
	if(EFI_ERROR(status))
	{
		printf("[BOOTMGR] [ERROR]: Failed to exit UEFI Boot services, error code: 0x%llX, mapKey: 0x%llX\r\n", status, mapKey);
		HaltSystem();
	}
	
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