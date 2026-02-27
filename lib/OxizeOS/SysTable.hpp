#pragma once
#include <stdint.h>

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
	uintptr_t DataAreaAddr;
	size_t DataAreaPageCount;
	uintptr_t TskSchlPhysAddr;
	uintptr_t TskSchlLoadSize;
	size_t KrnlMemRegionSize;
	uintptr_t regionStartPhys;
	uintptr_t NextPageTableFreePtr;
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
};