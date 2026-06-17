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
    // Iterate over XSDT Entries to find the MCFG Pointer

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

        // Check if entry is MCFG

        if(memcmp(entry->Signature, reinterpret_cast<const void*>(MCFGSignature), 4) != 0) {
            paging->FreeArea(reinterpret_cast<uintptr_t>(entry), 1);
            virtAlloc->FreeBlocks(virtAllocRes.value());
            continue;
        }

        // Map entire MCFG

        size_t mapPages = PAGE_ALIGN_UP(entry->Length) / PAGE_SIZE;

        paging->FreeArea(reinterpret_cast<uintptr_t>(entry), 1);
        virtAlloc->FreeBlocks(virtAllocRes.value());

        virtAllocRes = virtAlloc->AllocateBlocks(mapPages, MMD::VA_NODE_FLAG_MMIO | MMD::VA_NODE_FLAG_NO_EXECUTE_ACCESS | MMD::VA_NODE_FLAG_USED);
        if(!virtAllocRes)
        {
            printf("[SYSKRNL64] [ACPI] [ERROR]: Failed to allocate %d blocks for the MCFG ACPI entry\r\n", mapPages);
            return nullptr;
        }
        paging->MapArea(xsdt->entries[i], reinterpret_cast<uintptr_t>(virtAllocRes.value()), mapPages, PTE_PRESENT | PTE_RW | PTE_NX);
        this->mcfg = reinterpret_cast<MCFG*>(reinterpret_cast<uintptr_t>(virtAllocRes.value()) + (xsdt->entries[i] & 0xFFF));

        // Validate Checksum

        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(this->mcfg);
        uint8_t sum = 0;
        for(size_t i = 0; i < this->mcfg->sdt.Length; i++) sum += bytes[i];
        if(sum != 0) {
            printf("[SYSKRNL64] [ACPI] [ERROR]: MCFG is INVALID!\r\n");
            return nullptr;
        }

        return this->mcfg;
    }

    return nullptr;
}

MADT* ACPI::GetMADT()
{
    // Iterate over XSDT Entries to find the MADT Pointer

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

        // Check if entry is MADT

        if(memcmp(entry->Signature, reinterpret_cast<const void*>(MADTSignature), 4) != 0) {
            paging->FreeArea(reinterpret_cast<uintptr_t>(entry), 1);
            virtAlloc->FreeBlocks(virtAllocRes.value());
            continue;
        }

        // Map entire MADT

        size_t mapPages = PAGE_ALIGN_UP(entry->Length) / PAGE_SIZE;

        paging->FreeArea(reinterpret_cast<uintptr_t>(entry), 1);
        virtAlloc->FreeBlocks(virtAllocRes.value());

        virtAllocRes = virtAlloc->AllocateBlocks(mapPages, MMD::VA_NODE_FLAG_MMIO | MMD::VA_NODE_FLAG_NO_EXECUTE_ACCESS | MMD::VA_NODE_FLAG_USED);
        if(!virtAllocRes)
        {
            printf("[SYSKRNL64] [ACPI] [ERROR]: Failed to allocate %d blocks for MADT ACPI Entry\r\n", mapPages);
            return nullptr;
        }
        paging->MapArea(xsdt->entries[i], reinterpret_cast<uintptr_t>(virtAllocRes.value()), mapPages, PTE_PRESENT | PTE_RW | PTE_NX);
        this->madt = reinterpret_cast<MADT*>(reinterpret_cast<uintptr_t>(virtAllocRes.value()) + (xsdt->entries[i] & 0xFFF));

        // Validate Checksum

        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(this->madt);
        uint8_t sum = 0;
        for(size_t i = 0; i < this->madt->sdt.Length; i++) sum += bytes[i];
        if(sum != 0) {
            printf("[SYSKRNL64] [ACPI] [ERROR]: MDAT is INVALID!\r\n");
            return nullptr;
        }

        return this->madt;
    }

    return nullptr;
}