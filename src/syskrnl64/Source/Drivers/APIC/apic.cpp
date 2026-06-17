#include <Drivers/APIC/apic.hpp>

using namespace SysKrnl64::APIC;

APIC::APIC()
{
    // Manually create default constructor as compiler generated one causes #GP
    this->madt = nullptr;
}

APIC::APIC(ACPI::MADT* madt)
{
    if(!Initialize(madt)) HaltSystem();
}

bool APIC::Initialize(ACPI::MADT* madt)
{
    if(!madt) {
        printf("Invalid APIC Initializer args\r\n");
        return false;
    }

    this->madt = madt;

    // Disable legacy 8259 PIC to make sure that it doesn't send IRQs

    outb(MASTER_PIC_DATA_PORT, 0xFF);
    outb(SLAVE_PIC_DATA_PORT, 0xFF);

    // Check for X2APIC support from CPUID

    CPUID::CPUID_Regs regs = CPUID::GetCPUIDInfo(1);
    X2APICSupported = regs.rcx & (1 << 21);

    // Parse all MADT entries and store local copies of different entries in vectors

    uint8_t* buffer = reinterpret_cast<uint8_t*>(madt);

    uintptr_t positionInMADT = sizeof(ACPI::MADT);
    bool LAPICAddrOverrideExists = false;
    while(true)
    {
        uint8_t* entryPtr = buffer + positionInMADT;
        ACPI::MADTEntryHeader* hdr = reinterpret_cast<ACPI::MADTEntryHeader*>(entryPtr);
        switch(hdr->type)
        {
            case MADT_LocalAPIC:
            {
                ACPI::MADT_LAPIC* lapic = reinterpret_cast<ACPI::MADT_LAPIC*>(entryPtr);
                if(lapic->hdr.length < sizeof(ACPI::MADT_LAPIC)) {
                    printf("[SYSKRNL64] [APIC] [ERROR]: Invalid LAPIC Entry length 0x%llX at MADT offset 0x%llX\r\n", lapic->hdr.length, positionInMADT);
                    return false;
                }
                if((lapic->flags & 0x01) == 0) {
                    printf("[SYSKRNL64] [APIC] [WARN]: CPU Core with an APIC CPU ID of %d is disabled\r\n", lapic->apiccpuid);
                    break;
                }
                lapicEntries.push_back(*lapic);
                break;
            }
            case MADT_IOAPIC:
            {
                static size_t currentIoapic = 0;
                ACPI::MADT_IOAPIC* ioapic = reinterpret_cast<ACPI::MADT_IOAPIC*>(entryPtr);
                if(ioapic->hdr.length < sizeof(ACPI::MADT_IOAPIC)) {
                    printf("[SYSKRNL64] [APIC] [ERROR]: Invalid IOAPIC entry length 0x%llX at MADT offset 0x%llX\r\n", ioapic->hdr.length, positionInMADT);
                    return false;
                }
                if(ioapic->ioapicAddr == 0 || (ioapic->ioapicAddr & (PAGE_SIZE - 1)) != 0) {
                    printf("[SYSKRNL64] [APIC] [ERROR]: Null or misaligned IOAPIC Physical address 0x%llX at IOAPIC entry at MADT offset 0x%llX\r\n", ioapic->ioapicAddr, positionInMADT);
                    return false;
                }
                IOAPICDesc desc = {*ioapic, 0, 0, 0, 0};
                ioapicEntries[currentIoapic++] = desc;
                ioapicEntryCount++;
                break;
            }
            case MADT_InterruptSourceOverride:
            {
                ACPI::MADT_ISO* iso = reinterpret_cast<ACPI::MADT_ISO*>(entryPtr);
                if(iso->hdr.length < sizeof(ACPI::MADT_ISO)) {
                    printf("[SYSKRNL64] [APIC] [ERROR]: Invalid Interrupt Source Override entry length 0x%llX at MADT offset 0x%llX\r\n", iso->hdr.length, positionInMADT);
                    return false;
                }
                if(iso->IRQSource > 15) {
                    printf("[SYSKRNL64] [APIC] [ERROR]: IRQ Source %d for Interrupt Source Override entry is bigger than 15, ISO entry offset: 0x%llX\r\n", iso->IRQSource, positionInMADT);
                    return false;
                }
                isoEntries.push_back(*iso);
                break;
            }
            case MADT_LocalAPIC_AddressOverride:
            {
                if(LAPICAddrOverrideExists) {
                    printf("[SYSKRNL] [APIC] [WARN]: MADT Contains more than 1 LAPIC Address Override entries\r\n");
                    break;
                }
                ACPI::MADT_LAPIC_ADDR_OVERRIDE* lapicOverridePtr = reinterpret_cast<ACPI::MADT_LAPIC_ADDR_OVERRIDE*>(entryPtr);
                lapicOverride = *lapicOverridePtr;

                if(lapicOverride.hdr.length != sizeof(ACPI::MADT_LAPIC_ADDR_OVERRIDE)) {
                    printf("[SYSKRNL64] [APIC] [ERROR]: Invalid LAPIC Address Override entry length 0x%llX at MADT offset 0x%llX\r\n", lapicOverride.hdr.length, positionInMADT);
                    return false;
                }
                if(lapicOverride.lapicAddr == 0 || (lapicOverride.lapicAddr & (PAGE_SIZE - 1)) != 0) {
                    printf("[SYSKRNL64] [APIC] [ERROR]: Null or misaligned LAPIC Address Override physical address 0x%llX at LAPIC Address Override entry at MADT offset 0x%llX\r\n", lapicOverride.lapicAddr, positionInMADT);
                    return false;
                }
                LAPICAddrOverrideExists = true;
                break;
            }
            case MADT_ProcessorLocalX2APIC:
            {
                if(!X2APICSupported) {
                    printf("[SYSKRNL64] [APIC] [WARN]: X2APIC Entry at MADT offset 0x%llX present when CPU doesn't support X2APIC\r\n", positionInMADT);
                    break;
                }
                ACPI::MADT_X2APIC* x2Apic = reinterpret_cast<ACPI::MADT_X2APIC*>(entryPtr);
                if(x2Apic->hdr.length < sizeof(ACPI::MADT_X2APIC)) {
                    printf("[SYSKRNL64] [APIC] [ERROR]: Invalid X2APIC length 0x%llX at MADT offset 0x%llX\r\n", x2Apic->hdr.length, positionInMADT);
                    return false;
                }
                if((x2Apic->flags & 0x01) == 0) {
                    printf("[SYSKRNL64] [APIC] [WARN]: CPU Core with an X2APIC CPU ID of %d is disabled\r\n", x2Apic->apicProcID);
                    break;
                }
                x2ApicEntries.push_back(*x2Apic);
                break;
            }
            default: break;
        }

        positionInMADT += hdr->length;
        if(positionInMADT >= madt->sdt.Length) break; // End of entries
    }

    // Enable APIC & X2APIC(if supported) in APIC MSR

    uint64_t APICMSR = msr->ReadMSR(MSR_APIC_BASE);
    APICMSR |= (1ULL << 11);

    if(X2APICSupported) {
        APICMSR |= (1ULL << 10);
        printf("[SYSKRNL64] [APIC] [INFO]: X2APIC is supported\r\n");
    }

    msr->WriteMSR(MSR_APIC_BASE, APICMSR);

    if(X2APICSupported) {
        APICMSR = msr->ReadMSR(MSR_APIC_BASE);
        if((APICMSR & (1ULL << 10)) == 0) X2APICSupported = false; // APIC doesn't support X2APIC, even though CPUID lists it as supported
    }

    // Setup LAPIC

    if(!X2APICSupported)
    {
        uintptr_t lapicBasePhys = madt->lapicAddr;

        if(LAPICAddrOverrideExists) lapicBasePhys = lapicOverride.lapicAddr;

        auto virtAllocRes = virtAlloc->AllocateBlocks(1, MMD::VA_NODE_FLAG_MMIO | MMD::VA_NODE_FLAG_NO_EXECUTE_ACCESS | MMD::VA_NODE_FLAG_USED);
        if(!virtAllocRes)
        {
            printf("[SYSKRNL64] [APIC] [ERROR]: Failed to map LAPIC Base to memory\r\n");
            return false;
        }
        paging->MapArea(lapicBasePhys, reinterpret_cast<uintptr_t>(virtAllocRes.value()), 1, PTE_PRESENT | PTE_RW | PTE_PCD | PTE_NX);
        lapicBaseVirt = reinterpret_cast<uintptr_t>(virtAllocRes.value());
    }

    // Enable Spurious Vector Register and map it to ISR 255
    
    WriteLAPIC(0xF0, 0x100 | 0xFF);

    // Enable Task Priority Register

    WriteLAPIC(0x80, 0);

    // Clear Error Status Register

    WriteLAPIC(0x280, 0);
    ReadLAPIC(0x280);

    // Setup IOAPICs

    for(size_t i = 0; i < ioapicEntryCount; i++)
    {
        IOAPICDesc& ioapic = ioapicEntries[i];

        auto virtAllocRes = virtAlloc->AllocateBlocks(1, MMD::VA_NODE_FLAG_MMIO | MMD::VA_NODE_FLAG_NO_EXECUTE_ACCESS | MMD::VA_NODE_FLAG_USED);
        if(!virtAllocRes)
        {
            printf("[SYSKRNL64] [APIC] [ERROR]: Failed to map IOAPIC %d to memory\r\n", i);
            return false;
        }
        paging->MapArea(ioapic.entry.ioapicAddr, reinterpret_cast<uintptr_t>(virtAllocRes.value()), 1, PTE_PRESENT | PTE_RW | PTE_PCD | PTE_NX);
        ioapic.virt = reinterpret_cast<uintptr_t>(virtAllocRes.value());

        // Read reg 0x01
        uint32_t value = ReadIOAPIC(i, 1);
        ioapic.version = value & 0xFF;
        ioapic.maxRedirs = ((value >> 16) & 0xFF) + 1;

        // Read reg 0x00
        ioapic.id = (ReadIOAPIC(i, 0) >> 24) & 0x0F;
        
        printf("[SYSKRNL64] [APIC] [INFO]: Found an IOAPIC Entry %d with IOAPIC ID %d, version %d and maximum redirection entries of %d\r\n", i, ioapic.id, ioapic.version, ioapic.maxRedirs);

        // Disable all IOAPIC Inputs

        for(uint32_t j = 0; j < ioapic.maxRedirs; j++)
        {
            uint64_t entry = ReadIOAPIC64(i, 0x10 + j * 2);
            entry |= IOAPIC_MASK;
            WriteIOAPIC64(i, 0x10 + j * 2, entry);
        }

        for(uint32_t pin = 0; pin < ioapic.maxRedirs; pin++)
        {
            uint32_t gsi = ioapic.entry.globalSysInterruptBase + pin;
            if(GSIs.size() <= gsi) GSIs.resize(gsi + 1);
            GSIs[gsi] = {i, pin};
        }
    }

    // Remap IRQs ->GSIs

    // For some fucking reason APIC requires to remap IRQs to GSIs.
    // Why the fuck is that needed when it was perfectly fine with the legacy 8259-PIC(at least for me)?
    // Probably because of the fucking backwards compatibility which is rare to not find when developing on the x86 platform

    for(uint32_t i = 0; i < 16; i++) irqToGSI[i] = i;

    for(ACPI::MADT_ISO& iso : isoEntries)
    {
        if(iso.IRQSource > 15) continue;
        irqToGSI[iso.IRQSource] = iso.globalSysIntr;
    }

    // Initialize IRQs that we want
    InitializeIRQ(0);
    InitializeIRQ(1);

    return true;
}

uint32_t APIC::ReadLAPIC(uint32_t reg)
{
    if(X2APICSupported) return msr->ReadMSR(MSR_X2APIC_BASE + (reg >> 4));
    return *(volatile uint32_t*)(lapicBaseVirt + reg);
}

void APIC::WriteLAPIC(uint32_t reg, uint32_t value)
{
    if(X2APICSupported) msr->WriteMSR(MSR_X2APIC_BASE + (reg >> 4), value);
    else *(volatile uint32_t*)(lapicBaseVirt + reg) = value;
}

uint32_t APIC::ReadIOAPIC(int index, uint32_t reg)
{
    volatile uint32_t* base = reinterpret_cast<volatile uint32_t*>(ioapicEntries[index].virt);

    base[IOREGSEL] = reg;
    return base[IOWIN];
}

void APIC::WriteIOAPIC(int index, uint32_t reg, uint32_t value)
{
    volatile uint32_t* base = reinterpret_cast<volatile uint32_t*>(ioapicEntries[index].virt);

    base[IOREGSEL] = reg;
    base[IOWIN] = value;
}

uint64_t APIC::ReadIOAPIC64(int index, uint32_t reg)
{
    volatile uint32_t* base = reinterpret_cast<volatile uint32_t*>(ioapicEntries[index].virt);

    base[IOREGSEL] = reg;
    uint32_t low = base[IOWIN];

    base[IOREGSEL] = reg + 1;
    uint32_t high = base[IOWIN];

    return ((uint64_t)high << 32) | low;
}

void APIC::WriteIOAPIC64(int index, uint32_t reg, uint64_t value)
{
    volatile uint32_t* base = reinterpret_cast<volatile uint32_t*>(ioapicEntries[index].virt);

    base[IOREGSEL] = reg;
    base[IOWIN] = static_cast<uint32_t>(value & 0xFFFFFFFF);

    base[IOREGSEL] = reg + 1;
    base[IOWIN] = static_cast<uint32_t>(value >> 32);
}

GSIEntry APIC::ResolveGSI(uint32_t gsi)
{
    for(size_t i = 0; i < ioapicEntryCount; i++)
    {
        IOAPICDesc ioapic = ioapicEntries[i];
        if(gsi >= ioapic.entry.globalSysInterruptBase && gsi <= ioapic.entry.globalSysInterruptBase + ioapic.maxRedirs - 1) return { i, gsi - ioapic.entry.globalSysInterruptBase };
    }

    return {0, 0};
}

uint64_t APIC::BuildPolarityTrigger(uint16_t flags)
{
    uint64_t entry = 0;

    uint16_t polarity = flags & 0x03;
    uint16_t trigger = (flags >> 2) & 0x3;

    switch(polarity)
    {
        case 0:
        case 1: break;
        case 3: {
            entry |= IOAPIC_POLARITY;
            break;
        }
    }

    switch(trigger)
    {
        case 0:
        case 1: break;
        case 3: {
            entry |= IOAPIC_TRIGGER_MODE;
            break;
        }
    }

    return entry;
}

void APIC::SendEOI()
{
    WriteLAPIC(0xB0, 0);
}

void APIC::InitializeIRQ(uint8_t irq)
{
    uint32_t gsi = irqToGSI[irq];
    if(gsi >= GSIs.size()) return;

    GSIEntry e = GSIs[gsi];

    uint64_t entry = 0;

    uint8_t vector = IRQ_BASE + irq;

    uint16_t flags = 0;
    for(auto& iso : isoEntries)
    {
        if(iso.IRQSource == irq)
        {
            flags = iso.flags;
            break;
        }
    }

    uint64_t polarityTrigger = BuildPolarityTrigger(flags);

    uint32_t apicID = (ReadLAPIC(0x20) >> 24) & 0xFF;

    entry = (uint64_t)vector | IOAPIC_DELMODE_FIXED | IOAPIC_MASK | polarityTrigger | ((uint64_t)apicID << IOAPIC_DEST_SHIFT);

    WriteIOAPIC64(e.descIdx, 0x10 + e.pin * 2, entry);
}