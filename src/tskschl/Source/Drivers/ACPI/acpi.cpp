#include <Drivers/ACPI/acpi.hpp>

using namespace TskSchl::ACPI;

ACPI::ACPI(SystemTable* System)
{
    if(!Initialize(System)) HaltSystem();
}

bool ACPI::Initialize(SystemTable* System)
{
    // Map RSDP to virtual memory

    rsdp = reinterpret_cast<RSDP*>(mmd->malloc(1, MMD::MT_MMIO));

    if(!rsdp) {
        printf("[TSKSCHL] [ACPI] [ERROR]: Memory allocation failed\r\n");
        return false;
    }

    paging->MapArea(System->ACPI_RSDP, reinterpret_cast<uintptr_t>(rsdp), 1, PTE_PRESENT | PTE_RW | PTE_CD | PTE_NX);

    // Add the RSDP offset in page to the virtual address to prevent misalignment

    rsdp = reinterpret_cast<RSDP*>(reinterpret_cast<uint8_t*>(rsdp) + (System->ACPI_RSDP & 0xFFF));

    // Confirm that the RSDP is valid

    if(memcmp(rsdp->Signature, reinterpret_cast<const void*>(RSDPSignature), 8) != 0) {
        printf("[TSKSCHL] [ACPI] [ERROR]: Invalid RSDP Signature\r\n");
        return false;
    }

    // Confirm RSDP Checksum(first 20 bytes)

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(rsdp);
    uint8_t sum = 0;
    for(size_t i = 0; i < 20; i++) sum += bytes[i];
    if(sum != 0) {
        printf("[TSKSCHL] [ACPI] [ERROR]: RSDP is INVALID!\r\n");
        return false;
    }

    // Confirm ACPI Version is > 2, which it normally should be

    if(rsdp->Revision < 2) {
        printf("[TSKSCHL] [ACPI] [ERROR]: ACPI Version %u is lower than 2.0\r\n", rsdp->Revision);
        return false;
    }

    // Confirm XSDP Checksum(all bytes)

    sum = 0;
    for(size_t i = 0; i < rsdp->Length; i++) sum += bytes[i];
    if(sum != 0) {
        printf("[TSKSCHL] [ACPI] [ERROR]: XSDP is INVALID!\r\n");
        return false;
    }
    
    // Map XSDT to virtual memory

    xsdt = reinterpret_cast<XSDT*>(mmd->malloc(1, MMD::MT_MMIO));

    if(!xsdt) {
        printf("[TSKSCHL] [ACPI] [ERROR]: Memory allocation failed\r\n");
        return false;
    }

    paging->MapArea(rsdp->XsdtAddress, reinterpret_cast<uintptr_t>(xsdt), 1, PTE_PRESENT | PTE_RW | PTE_NX);

    xsdt = reinterpret_cast<XSDT*>(reinterpret_cast<uint8_t*>(xsdt) + (rsdp->XsdtAddress & 0xFFF));

    size_t mapPages = PAGE_ALIGN_UP(xsdt->sdt.Length) / PAGE_SIZE;

    mmd->freeBlocks(1, MMD::MT_MMIO);

    xsdt = reinterpret_cast<XSDT*>(mmd->malloc(mapPages, MMD::MT_MMIO));

    if(!xsdt) {
        printf("[TSKSCHL] [ACPI] [ERROR]: Memory allocation failed\r\n");
        return false;
    }

    paging->MapArea(rsdp->XsdtAddress, reinterpret_cast<uintptr_t>(xsdt), mapPages, PTE_PRESENT | PTE_RW | PTE_NX);

    // Add the XSDT offset in page to the virtual address to prevent misalignment

    xsdt = reinterpret_cast<XSDT*>(reinterpret_cast<uint8_t*>(xsdt) + (rsdp->XsdtAddress & 0xFFF));

    // Confirm whether the XSDT is genuine

    if(memcmp(xsdt->sdt.Signature, reinterpret_cast<const void*>(XSDTSignature), 4) != 0) {
        printf("[TSKSCHL] [ACPI] [ERROR]: Invalid XSDT Signature\r\n");
        return false;
    }

    bytes = reinterpret_cast<const uint8_t*>(xsdt);
    sum = 0;
    for(size_t i = 0; i < xsdt->sdt.Length; i++) sum += bytes[i];
    if(sum != 0) {
        printf("[TSKSCHL] [ACPI] [ERROR]: XSDT is INVALID!\r\n");
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
        ACPISDTHeader* entry = reinterpret_cast<ACPISDTHeader*>(mmd->malloc(1, MMD::MT_MMIO));

        if(!entry) {
            printf("[TSKSCHL] [ACPI] [ERROR]: Memory allocation failed\r\n");
            return nullptr;
        }

        paging->MapArea(xsdt->entries[i], reinterpret_cast<uintptr_t>(entry), 1, PTE_PRESENT | PTE_RW | PTE_NX);

        // Check if entry is MCFG

        if(memcmp(entry->Signature, reinterpret_cast<const void*>(MCFGSignature), 4) != 0) {
            mmd->freeBlocks(1, MMD::MT_MMIO);
            continue;
        }

        // Map entire MCFG

        size_t mapPages = PAGE_ALIGN_UP(entry->Length) / PAGE_SIZE;

        mmd->freeBlocks(1, MMD::MT_MMIO);
        
        this->mcfg = reinterpret_cast<MCFG*>(mmd->malloc(mapPages, MMD::MT_MMIO));

        if(!this->mcfg) {
            printf("[TSKSCHL] [ACPI] [ERROR]: Memory allocation failed\r\n");
            return nullptr;
        }

        paging->MapArea(xsdt->entries[i], reinterpret_cast<uintptr_t>(this->mcfg), mapPages, PTE_PRESENT | PTE_RW | PTE_NX);

        // Validate Checksum

        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(this->mcfg);
        uint8_t sum = 0;
        for(size_t i = 0; i < mcfg->sdt.Length; i++) sum += bytes[i];
        if(sum != 0) {
            printf("[TSKSCHL] [ACPI] [ERROR]: MCFG is INVALID!\r\n");
            return nullptr;
        }

        return this->mcfg;
    }

    return nullptr;
}