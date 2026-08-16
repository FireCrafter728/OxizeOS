// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |----------------------------------------------------------------------------------------------------------------------------------| //
// | OxizeOS Kernel Implementation                                                                                                    | //
// | SysTable: contains various enums and structures for the System Table provided by the Boot Manager and used by the OxizeOS Kernel | //
// |----------------------------------------------------------------------------------------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdint.h>
#include <stddef.h>

enum MemoryRegionType : uint8_t
{
	MEMTYPE_USABLE = 0x01,
	MEMTYPE_HARDWARE_RESERVED,
	MEMTYPE_SOFTWARE_RESERVED,
	MEMTYPE_ACPIRECLAIM,
	MEMTYPE_ACPINVS,
	MEMTYPE_BAD_MEMORY,
};

struct MemoryRegion
{
	MemoryRegionType type;
	uintptr_t phys;
	size_t length;
};

struct MemoryTable
{
	size_t regionCount;
	MemoryRegion* regions;
};

enum PixelFormats : uint32_t
{
	PIXEL_FORMAT_R8G8B8A8,
	PIXEL_FORMAT_R8G8B8,
	PIXEL_FORMAT_B8G8R8A8,
	PIXEL_FORMAT_B8G8R8,
};

struct VideoResolution
{
	size_t resWidth, resHeight;
	PixelFormats PixelFormat;
	size_t resPitch;
	uint32_t gopIndex;
};

struct GOPFramebuffer
{
	uintptr_t fbBase;
	VideoResolution currentResolution;
};

struct KernelMemoryLayout
{
	uintptr_t PageTableAddr;
	size_t PageTablePageCount;
	uintptr_t StackAddr;
	size_t StackPageCount;
	uintptr_t SysKrnl64PhysAddr;
	uintptr_t SysKrnl64LoadSize;
	size_t KrnlMemRegionSize;
	uintptr_t regionStartPhys;
	uintptr_t NextPageTableFreePtr;
	uintptr_t PhysAllocBitmapAddr;
	size_t PhysAllocBitmapPages;
	uintptr_t SMPThreadBringupPageAddr;
};

struct KernelStructureRegion
{
	uintptr_t phys, virt;
	size_t totalPages;
	bool guardPage;
	bool writeProtected, execProtected;
};

// Uses a custom epoch, which is January 1st, 2000th year, UTC 00:00:00
struct SystemTime
{
	uint64_t SecondsSinceEpoch;
	uint32_t Nanoseconds;
};

struct BootTimestamp
{
	SystemTime systemTime;
	uint64_t tscCounter;
};

struct SystemTable
{
	MemoryTable memTable;
	uintptr_t ACPI_RSDP;
	uintptr_t SMBIOS_PTR;
	uint8_t SMBIOS_VersionMajor;
	uint8_t SMBIOS_VersionMinor;
	GOPFramebuffer fb;
	KernelMemoryLayout memLayout;
	uint64_t usableRAMPages;
	KernelStructureRegion kernelStructureRegions[10];
	BootTimestamp bootTime;
	uintptr_t ResourceSectionVirtAddr;
	uintptr_t ResourceSectionRootDirectoryAddress;
};