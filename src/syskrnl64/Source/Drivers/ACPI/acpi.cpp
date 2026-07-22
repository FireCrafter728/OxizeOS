// SPDX-License-Identifier: GPL-3.0-or-later

#include <Drivers/ACPI/acpi.hpp>

using namespace SysKrnl64::ACPI;

ACPI::ACPI(SystemTable* System)
{
    if(!Initialize(System)) HaltSystem();
}

bool ACPI::Initialize(SystemTable* System)
{
    // Map RSDP to virtual memory

    auto rsdpAllocRes = virtAlloc->AllocateBlocks(1, MMD::VA_NODE_FLAG_MMIO | MMD::VA_NODE_FLAG_NO_EXECUTE_ACCESS | MMD::VA_NODE_FLAG_USED);
    if(!rsdpAllocRes)
    {
        printf("[SYSKRNL64] [ACPI] [ERROR]: Failed to allocate virtual memory for RSDP, error code: %d\r\n", rsdpAllocRes.error());
        return false;
    }
    paging->MapArea(System->ACPI_RSDP, reinterpret_cast<uintptr_t>(rsdpAllocRes.value()), 1, PTE_PRESENT | PTE_RW | PTE_NX);
    rsdp = reinterpret_cast<RSDP*>(reinterpret_cast<uintptr_t>(rsdpAllocRes.value()) + (System->ACPI_RSDP & 0xFFF));

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

    auto xsdtAllocRes = virtAlloc->AllocateBlocks(1, MMD::VA_NODE_FLAG_MMIO | MMD::VA_NODE_FLAG_NO_EXECUTE_ACCESS | MMD::VA_NODE_FLAG_USED);
    if(!xsdtAllocRes)
    {
        printf("[SYSKRNL64] [ACPI] [ERROR]: Failed to allocate virtual memory for XSDT, error code: %d\r\n", xsdtAllocRes.error());
        return false;
    }
    paging->MapArea(rsdp->XsdtAddress, reinterpret_cast<uintptr_t>(xsdtAllocRes.value()), 1, PTE_PRESENT | PTE_RW | PTE_NX);
    xsdt = reinterpret_cast<XSDT*>(reinterpret_cast<uintptr_t>(xsdtAllocRes.value()) + (rsdp->XsdtAddress & 0xFFF));

    size_t mapPages = PAGE_ALIGN_UP(xsdt->sdt.Length) / PAGE_SIZE;

    if(mapPages > 1)
    {
        paging->FreeArea(reinterpret_cast<uintptr_t>(xsdt), 1);
        virtAlloc->FreeBlocks(xsdt);

        xsdtAllocRes = virtAlloc->AllocateBlocks(mapPages, MMD::VA_NODE_FLAG_MMIO | MMD::VA_NODE_FLAG_NO_EXECUTE_ACCESS | MMD::VA_NODE_FLAG_USED);
        if(!xsdtAllocRes)
        {
            printf("[SYSKRNL64] [ACPI] [ERROR]: Failed to allocate virtual memory for XSDT, error code: %d\r\n", xsdtAllocRes.error());
            return false;
        }
        paging->MapArea(rsdp->XsdtAddress, reinterpret_cast<uintptr_t>(xsdtAllocRes.value()), mapPages, PTE_PRESENT | PTE_RW | PTE_NX);
        xsdt = reinterpret_cast<XSDT*>(reinterpret_cast<uintptr_t>(xsdtAllocRes.value()) + (rsdp->XsdtAddress & 0xFFF));
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

    xsdtEntries = (xsdt->sdt.Length - sizeof(ACPISDTHeader)) / 8;

    return true;
}

MCFG* ACPI::GetMCFG()
{
    if(this->mcfg) return this->mcfg;
    ACPISDTHeader* hdr = GetMappedStructure(MCFGSignature);
    if(!hdr)
    {
        printf("[SYSKRNL64] [ACPI] [ERROR]: No MCFG Structure present in the ACPI\r\n");
        return nullptr;
    }

    this->mcfg = reinterpret_cast<MCFG*>(hdr);
    return this->mcfg;
}

MADT* ACPI::GetMADT()
{
    if(this->madt) return this->madt;
    ACPISDTHeader* hdr = GetMappedStructure(MADTSignature);
    if(!hdr)
    {
        printf("[SYSKRNL64] [ACPI] [ERROR]: No MADT Structure present in the ACPI\r\n");
        return nullptr;
    }

    this->madt = reinterpret_cast<MADT*>(hdr);
    return this->madt;
}

HPET* ACPI::GetHPET()
{
    if(this->hpet) return this->hpet;
    ACPISDTHeader* hdr = GetMappedStructure(HPETSignature);
    if(!hdr)
    {
        printf("[SYSKRNL64] [ACPI] [ERROR]: No HPET Structure present in the ACPI\r\n");
        return nullptr;
    }

    this->hpet = reinterpret_cast<HPET*>(hdr);
    return this->hpet;
}

ACPISDTHeader* ACPI::GetMappedStructure(uint32_t signature)
{
    // Iterate over XSDT Entries to find the first entry with the correct signature

    for(size_t i = 0; i < xsdtEntries; i++)
    {
        // Map entry

        auto virtAllocRes = virtAlloc->AllocateBlocks(1, MMD::VA_NODE_FLAG_MMIO | MMD::VA_NODE_FLAG_NO_EXECUTE_ACCESS | MMD::VA_NODE_FLAG_USED);
        if(!virtAllocRes)
        {
            printf("[SYSKRNL64] [ACPI] [ERROR]: Failed to allocate memory for a XSDT entry\r\n");
            return nullptr;
        }
        paging->MapArea(xsdt->entries[i], reinterpret_cast<uintptr_t>(virtAllocRes.value()), 1, PTE_PRESENT | PTE_RW | PTE_NX);
        ACPISDTHeader* entry = reinterpret_cast<ACPISDTHeader*>(reinterpret_cast<uintptr_t>(virtAllocRes.value()) + (xsdt->entries[i] & 0xFFF));

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

        virtAllocRes = virtAlloc->AllocateBlocks(mapPages, MMD::VA_NODE_FLAG_MMIO | MMD::VA_NODE_FLAG_NO_EXECUTE_ACCESS | MMD::VA_NODE_FLAG_USED);
        if(!virtAllocRes)
        {
            printf("[SYSKRNL64] [ACPI] [ERROR]: Failed to allocate %d blocks for a XSDT Entry\r\n", mapPages);
            return nullptr;
        }
        paging->MapArea(xsdt->entries[i], reinterpret_cast<uintptr_t>(virtAllocRes.value()), mapPages, PTE_PRESENT | PTE_RW | PTE_NX);
        entry = reinterpret_cast<ACPISDTHeader*>(reinterpret_cast<uintptr_t>(virtAllocRes.value()) + (xsdt->entries[i] & 0xFFF));

        // Validate Checksum

        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(entry);
        uint8_t sum = 0;
        for(size_t i = 0; i < entry->Length; i++) sum += bytes[i];
        if(sum != 0) {
            printf("[SYSKRNL64] [ACPI] [ERROR]: XSDT Entry %d is INVALID!\r\n", i);
            return nullptr;
        }

        return entry;
    }

    return nullptr;
}