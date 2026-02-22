#pragma once

#include <crt.hpp>
#include <io.hpp>
#include <FileSystem.hpp>
#include <string.hpp>
#include <SysTable.hpp>
#include <SysTableBuilder.hpp>
#include <stdio.hpp>
#include <algorithm>
#include <elf.hpp>

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/AcpiTable.h>
#include <IndustryStandard/Acpi.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/Smbios.h>
#include <Guid/Acpi.h>
#include <Guid/SmBios.h>

constexpr size_t PAGE_SIZE = 0x1000;

#define PACK __attribute__((packed))
#define FPACK __attribute__((packed, aligned(1)))

#define PAGE_ALIGN_UP(addr) (((addr) + 0xFFFULL) & ~(0xFFFULL))
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(0xFFFULL))

namespace BootMgr
{
	extern EFI_SYSTEM_TABLE* gSystem;

	bool CompareGUID(EFI_GUID guid1, EFI_GUID guid2);

	enum PTE : uint64_t
	{
		PTE_PRESENT = (1ULL << 0),
		PTE_RW = (1ULL << 1ULL),
		PTE_USER = (1ULL << 2),
		PTE_WC = (1ULL << 3),
		PTE_CD = (1ULL << 4),
		PTE_ACCESSED = (1ULL << 5),
		PTE_NX = (1ULL << 63),

		PTE_PHYS_MASK = 0x000FFFFFFFFFF000,
	};

	inline uint64_t MAKE_PTE(uintptr_t Addr, uint64_t flags) {
		return (Addr & PTE_PHYS_MASK) | flags;
	}

	enum CR3 : uint64_t
	{
		CR3_WC = (1ULL << 3),
		CR3_CD = (1ULL << 4),
		CR3_LAM57 = (1ULL << 61),
		CR3_LAM48 = (1ULL << 62),

		CR3_PHYS_MASK = 0x000FFFFFFFFFF000,
	};

	inline uint64_t MAKE_CR3(uintptr_t Addr, uint64_t flags) {
		return (Addr & CR3_PHYS_MASK) | flags;
	}

	constexpr uintptr_t MapAddr = 0xFFFF800000000000;
}