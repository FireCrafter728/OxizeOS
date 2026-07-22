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

#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |----------------------------------------------------------------------------------------------------------------------------------| //
// | OxizeOS Kernel Implementation                                                                                                    | //
// | SysTable: contains various enums and structures for the System Table provided by the Boot Manager and used by the OxizeOS Kernel | //
// |----------------------------------------------------------------------------------------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

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
};