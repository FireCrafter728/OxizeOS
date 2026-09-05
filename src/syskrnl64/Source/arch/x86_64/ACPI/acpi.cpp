// SPDX-License-Identifier: GPL-3.0-or-later

#include <arch/x86_64/ACPI/acpi.hpp>

#include <arch/x86_64/Utility/io.hpp>
#include <main/utils.hpp>
#include <stdio.hpp>
#include <string.hpp>

using namespace krnl;

bool ACPI::Initialize(SystemTable* System)
{
	// Map RSDP to virtual memory

	auto rsdpAllocRes = virtAlloc->AllocateBlocks(1, VA_NODE_FLAG_MMIO | VA_NODE_FLAG_NO_EXECUTE_ACCESS | VA_NODE_FLAG_USED);
	if(!rsdpAllocRes)
	{
		printf("[SYSKRNL64] [ACPI] [ERROR]: Failed to allocate virtual memory for RSDP, error code: %lu\r\n", rsdpAllocRes.error());
		return false;
	}
	paging->MapArea(System->ACPI_RSDP, reinterpret_cast<uintptr_t>(rsdpAllocRes.value()), 1, PTE_PRESENT | PTE_RW | PTE_NX);
	rsdp = reinterpret_cast<ACPI_RSDP*>(reinterpret_cast<uintptr_t>(rsdpAllocRes.value()) + (System->ACPI_RSDP & 0xFFF));

	// Confirm that the RSDP is valid

	if(memcmp(rsdp->Signature, reinterpret_cast<const void*>(RSDPSignature), 8) != 0) {
		printf("[SYSKRNL64] [ACPI] [ERROR]: Invalid RSDP Signature\r\n");
		return false;
	}

	// Confirm RSDP Checksum(first 20 bytes)

	const uint8_t* bytes = reinterpret_cast<const uint8_t*>(rsdp);
	uint8_t sum = 0;
	for(size_t i = 0; i < 20; i++) sum += bytes[i];
	if(sum != 0) {
		printf("[SYSKRNL64] [ACPI] [ERROR]: RSDP is INVALID!\r\n");
		return false;
	}

	// Confirm ACPI Version is > 2, which it normally should be

	if(rsdp->Revision < 2) {
		printf("[SYSKRNL64] [ACPI] [ERROR]: ACPI Version %u is lower than 2.0\r\n", rsdp->Revision);
		return false;
	}

	// Confirm XSDP Checksum(all bytes)

	sum = 0;
	for(size_t i = 0; i < rsdp->Length; i++) sum += bytes[i];
	if(sum != 0) {
		printf("[SYSKRNL64] [ACPI] [ERROR]: XSDP is INVALID!\r\n");
		return false;
	}
	
	// Map XSDT to virtual memory

	auto xsdtAllocRes = virtAlloc->AllocateBlocks(1, VA_NODE_FLAG_MMIO | VA_NODE_FLAG_NO_EXECUTE_ACCESS | VA_NODE_FLAG_USED);
	if(!xsdtAllocRes)
	{
		printf("[SYSKRNL64] [ACPI] [ERROR]: Failed to allocate virtual memory for XSDT, error code: %lu\r\n", xsdtAllocRes.error());
		return false;
	}
	paging->MapArea(rsdp->XsdtAddress, reinterpret_cast<uintptr_t>(xsdtAllocRes.value()), 1, PTE_PRESENT | PTE_RW | PTE_NX);
	xsdt = reinterpret_cast<ACPI_XSDT*>(reinterpret_cast<uintptr_t>(xsdtAllocRes.value()) + (rsdp->XsdtAddress & 0xFFF));

	size_t mapPages = PAGE_ALIGN_UP(xsdt->sdt.Length) / PAGE_SIZE;

	if(mapPages > 1)
	{
		paging->FreeArea(reinterpret_cast<uintptr_t>(xsdt), 1);
		virtAlloc->FreeBlocks(xsdt);

		xsdtAllocRes = virtAlloc->AllocateBlocks(mapPages, VA_NODE_FLAG_MMIO | VA_NODE_FLAG_NO_EXECUTE_ACCESS | VA_NODE_FLAG_USED);
		if(!xsdtAllocRes)
		{
			printf("[SYSKRNL64] [ACPI] [ERROR]: Failed to allocate virtual memory for XSDT, error code: %lu\r\n", xsdtAllocRes.error());
			return false;
		}
		paging->MapArea(rsdp->XsdtAddress, reinterpret_cast<uintptr_t>(xsdtAllocRes.value()), mapPages, PTE_PRESENT | PTE_RW | PTE_NX);
		xsdt = reinterpret_cast<ACPI_XSDT*>(reinterpret_cast<uintptr_t>(xsdtAllocRes.value()) + (rsdp->XsdtAddress & 0xFFF));
	}

	// Confirm whether the XSDT is genuine

	if(memcmp(xsdt->sdt.Signature, reinterpret_cast<const void*>(XSDTSignature), 4) != 0) {
		printf("[SYSKRNL64] [ACPI] [ERROR]: Invalid XSDT Signature\r\n");
		return false;
	}

	bytes = reinterpret_cast<const uint8_t*>(xsdt);
	sum = 0;
	for(size_t i = 0; i < xsdt->sdt.Length; i++) sum += bytes[i];
	if(sum != 0) {
		printf("[SYSKRNL64] [ACPI] [ERROR]: XSDT is INVALID!\r\n");
		return false;
	}

	// Store Variables

	xsdtEntries = (xsdt->sdt.Length - sizeof(ACPI_SDTHeader)) / 8;

	return true;
}

ACPI_MCFG* ACPI::GetMCFG()
{
	if(this->mcfg) return this->mcfg;
	ACPI_SDTHeader* hdr = GetMappedStructure(MCFGSignature);
	if(!hdr)
	{
		printf("[SYSKRNL64] [ACPI] [ERROR]: No MCFG Structure present in the ACPI\r\n");
		return nullptr;
	}

	this->mcfg = reinterpret_cast<ACPI_MCFG*>(hdr);
	return this->mcfg;
}

ACPI_MADT* ACPI::GetMADT()
{
	if(this->madt) return this->madt;
	ACPI_SDTHeader* hdr = GetMappedStructure(MADTSignature);
	if(!hdr)
	{
		printf("[SYSKRNL64] [ACPI] [ERROR]: No MADT Structure present in the ACPI\r\n");
		return nullptr;
	}

	this->madt = reinterpret_cast<ACPI_MADT*>(hdr);
	return this->madt;
}

ACPI_HPET* ACPI::GetHPET()
{
	if(this->hpet) return this->hpet;
	ACPI_SDTHeader* hdr = GetMappedStructure(HPETSignature);
	if(!hdr)
	{
		printf("[SYSKRNL64] [ACPI] [ERROR]: No HPET Structure present in the ACPI\r\n");
		return nullptr;
	}

	this->hpet = reinterpret_cast<ACPI_HPET*>(hdr);
	return this->hpet;
}

ACPI_SDTHeader* ACPI::GetMappedStructure(uint32_t signature)
{
	// Iterate over XSDT Entries to find the first entry with the correct signature

	for(size_t i = 0; i < xsdtEntries; i++)
	{
		// Map entry

		auto virtAllocRes = virtAlloc->AllocateBlocks(1, VA_NODE_FLAG_MMIO | VA_NODE_FLAG_NO_EXECUTE_ACCESS | VA_NODE_FLAG_USED);
		if(!virtAllocRes)
		{
			printf("[SYSKRNL64] [ACPI] [ERROR]: Failed to allocate memory for a XSDT entry\r\n");
			return nullptr;
		}
		paging->MapArea(xsdt->entries[i], reinterpret_cast<uintptr_t>(virtAllocRes.value()), 1, PTE_PRESENT | PTE_RW | PTE_NX);
		ACPI_SDTHeader* entry = reinterpret_cast<ACPI_SDTHeader*>(reinterpret_cast<uintptr_t>(virtAllocRes.value()) + (xsdt->entries[i] & 0xFFF));

		// Check if entry is the one we're searching for

		uint32_t entrySig32 = *reinterpret_cast<uint32_t*>(&entry->Signature);

		if(entrySig32 != signature) {
			paging->FreeArea(PAGE_ALIGN_DOWN(reinterpret_cast<uintptr_t>(entry)), 1);
			virtAlloc->FreeBlocks(virtAllocRes.value());
			continue;
		}

		// Map the entire structure

		size_t mapPages = PAGE_ALIGN_UP(entry->Length) / PAGE_SIZE;

		paging->FreeArea(PAGE_ALIGN_DOWN(reinterpret_cast<uintptr_t>(entry)), 1);
		virtAlloc->FreeBlocks(virtAllocRes.value());

		virtAllocRes = virtAlloc->AllocateBlocks(mapPages, VA_NODE_FLAG_MMIO | VA_NODE_FLAG_NO_EXECUTE_ACCESS | VA_NODE_FLAG_USED);
		if(!virtAllocRes)
		{
			printf("[SYSKRNL64] [ACPI] [ERROR]: Failed to allocate %llu blocks for a XSDT Entry\r\n", mapPages);
			return nullptr;
		}
		paging->MapArea(xsdt->entries[i], reinterpret_cast<uintptr_t>(virtAllocRes.value()), mapPages, PTE_PRESENT | PTE_RW | PTE_NX);
		entry = reinterpret_cast<ACPI_SDTHeader*>(reinterpret_cast<uintptr_t>(virtAllocRes.value()) + (xsdt->entries[i] & 0xFFF));

		// Validate Checksum

		const uint8_t* bytes = reinterpret_cast<const uint8_t*>(entry);
		uint8_t sum = 0;
		for(size_t i = 0; i < entry->Length; i++) sum += bytes[i];
		if(sum != 0) {
			printf("[SYSKRNL64] [ACPI] [ERROR]: XSDT Entry %llu is INVALID!\r\n", i);
			return nullptr;
		}

		return entry;
	}

	return nullptr;
}