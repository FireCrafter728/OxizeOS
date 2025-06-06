#include <ACPI/acpi.hpp>
#include <string.h>
#include <io.h>
#include <stdio.h>

using namespace x86Kernel::ACPI;

#define ACPI_MADT_SIGNATURE         "APIC"

#define ACPI_EBDA_SEGMENT           0x40E
#define ACPI_EBDA_SIZE              0x400

#define ACPI_BIOS_START             0x000E0000
#define ACPI_BIOS_END               0x00100000

bool ValidateChecksum(const uint8_t* addr, size_t length)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += addr[i];
    }
    return (sum == 0);
}

const RSDPDescV2* FindRSDPInRange(uintptr_t start, size_t length)
{
    constexpr char sig[8] = {'R', 'S', 'D', ' ', 'P', 'T', 'R', ' '};
    for (uintptr_t addr = start; addr < start + length; addr += 16) {
        const char* ptr = reinterpret_cast<const char*>(addr);
        if (memcmp(ptr, sig, 8) == 0) {
            const RSDPDescV2* desc = reinterpret_cast<const RSDPDescV2*>(ptr);
            size_t len = (desc->v1.Revision >= 2) ? desc->Length : sizeof(RSDPDescV1);
            if (ValidateChecksum(reinterpret_cast<const uint8_t*>(ptr), len)) {
                return desc;
            }
        }
    }
    return nullptr;
}

uintptr_t GetEBDAAddr()
{
    volatile uint16_t* ebdaSeg = reinterpret_cast<volatile uint16_t*>(ACPI_EBDA_SEGMENT);
    return static_cast<uintptr_t>(*ebdaSeg) << 4;
}

ACPI::ACPI(ACPIDesc* desc)
{
    if (!Initialize(desc)) {
        printf("[ACPI] [ERROR]: Failed to parse ACPI tables\r\n");
        HaltSystem();
    }
}

bool ACPI::Initialize(ACPIDesc* desc)
{
    this->desc = desc;
    this->desc->rsdp = GetRSDPDesc();
    if(this->desc->rsdp == nullptr) return false;
    if (!ParseMADT(&desc->deviceList)) return false;
    return true;
}

const RSDPDescV2* ACPI::GetRSDPDesc()
{
    const RSDPDescV2* rsdp = nullptr;

    uintptr_t ebdaAddr = GetEBDAAddr();
    if (ebdaAddr) {
        rsdp = FindRSDPInRange(ebdaAddr, ACPI_EBDA_SIZE);
    }

    if (!rsdp) {
        rsdp = FindRSDPInRange(ACPI_BIOS_START, ACPI_BIOS_END - ACPI_BIOS_START);
    }

    return rsdp;
}

const ACPISDTHeader* ACPI::FindTable(const char* signature)
{
    if(!desc->rsdp) return nullptr;

    if(desc->rsdp->v1.Revision < 2) {
        const RSDT* rsdt = reinterpret_cast<const RSDT*>(desc->rsdp->v1.RSDTAddr);
        size_t count = (rsdt->header.Length - sizeof(ACPISDTHeader)) / sizeof(uint32_t);

        for(size_t i = 0; i < count; i++) {
            const ACPISDTHeader* table = reinterpret_cast<const ACPISDTHeader*>((uintptr_t)rsdt->Ptr[i]);
            if(memcmp(table->Signature, signature, 4) == 0) return table;
        }
    }
    else {
        const XSDT* xsdt = reinterpret_cast<const XSDT*>(desc->rsdp->XsdtAddress);
        size_t count = (xsdt->header.Length - sizeof(ACPISDTHeader)) / sizeof(uint64_t);

        for(size_t i = 0; i < count; i++) {
            const ACPISDTHeader* table = reinterpret_cast<const ACPISDTHeader*>((uintptr_t)xsdt->Ptr[i]);
            if(memcmp(table->Signature, signature, 4) == 0) return table;
        }
    }
    return nullptr;
}

bool ACPI::ParseMADT(MADTDeviceList* list)
{
    const MADT* madt = reinterpret_cast<const MADT*>(FindTable("APIC"));
    if(!madt) return false;

    uintptr_t ptr = reinterpret_cast<uintptr_t>(madt) + sizeof(MADT);
    uintptr_t end = reinterpret_cast<uintptr_t>(madt) + madt->header.Length;

    while(ptr < end) {
        const MADTEntryHeader* entry = reinterpret_cast<const MADTEntryHeader*>(ptr);

        switch(entry->Type)
        {
        case static_cast<uint8_t>(MADTEntryType::LocalAPIC):
        {
            if(list->lapicCount < 32) memcpy(&list->lapics[list->lapicCount++], entry, sizeof(MADTLocalAPIC));
            break;
        }
        case static_cast<uint8_t>(MADTEntryType::IOAPIC):
        {
            if(list->ioapicCount < 8) memcpy(&list->ioapics[list->ioapicCount++], entry, sizeof(MADTIOAPIC));
            break;
        }
        case static_cast<uint8_t>(MADTEntryType::InterruptSourceOverride):
        {
            if(list->overrideCount < 16) memcpy(&list->overrides[list->overrideCount++], entry, sizeof(MADTInterruptSourceOverride));
            break;
        }
        }
        ptr += entry->Length;
    }

    return true;
}