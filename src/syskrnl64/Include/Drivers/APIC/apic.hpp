#pragma once

#include <Drivers/ACPI/acpi.hpp>

#include <stdint.hpp>

namespace SysKrnl64
{
    namespace APIC
    {
        constexpr uint16_t MASTER_PIC_COMMAND_PORT = 0x20;
        constexpr uint16_t SLAVE_PIC_COMMAND_PORT = 0xA0;
        constexpr uint16_t MASTER_PIC_DATA_PORT = 0x21;
        constexpr uint16_t SLAVE_PIC_DATA_PORT = 0xA1;

        constexpr uint8_t MADT_LocalAPIC = 0;
        constexpr uint8_t MADT_IOAPIC = 1;
        constexpr uint8_t MADT_InterruptSourceOverride = 2;
        constexpr uint8_t MADT_NMI_Source = 3;
        constexpr uint8_t MADT_LocalAPIC_NMI = 4;
        constexpr uint8_t MADT_LocalAPIC_AddressOverride = 5;
        constexpr uint8_t MADT_IO_SAPIC = 6;
        constexpr uint8_t MADT_Local_SAPIC = 7;
        constexpr uint8_t MADT_PlatformInterruptSource = 8;
        constexpr uint8_t MADT_ProcessorLocalX2APIC = 9;
        constexpr uint8_t MADT_LocalX2APIC_NMI = 10;
        constexpr uint8_t MADT_GIC_CPU_Interface = 11;
        constexpr uint8_t MADT_GIC_Distributor = 12;
        constexpr uint8_t MADT_GIC_MSI_Frame = 13;
        constexpr uint8_t MADT_GIC_Redistrubutor = 14;
        constexpr uint8_t MADT_GIC_InterruptTranslationService = 15;
        constexpr uint8_t MADT_MultiprocessorWakeup = 16;

        constexpr uintptr_t IOREGSEL = 0x00 / 4;
        constexpr uintptr_t IOWIN = 0x10 / 4;

        constexpr uint64_t IOAPIC_VECTOR_MASK   = 0xFFULL;

        constexpr uint64_t IOAPIC_DELMODE_SHIFT = 8;
        constexpr uint64_t IOAPIC_DELMODE_MASK  = (7ULL << 8);

        constexpr uint64_t IOAPIC_DESTMODE      = (1ULL << 11);
        constexpr uint64_t IOAPIC_POLARITY      = (1ULL << 13);
        constexpr uint64_t IOAPIC_TRIGGER_MODE  = (1ULL << 15);
        constexpr uint64_t IOAPIC_MASK          = (1ULL << 16);

        constexpr uint64_t IOAPIC_DEST_SHIFT    = 56;
        constexpr uint64_t IOAPIC_DEST_MASK     = (0xFFULL << 56);

        constexpr uint64_t IOAPIC_DELMODE_FIXED  = (0ULL << 8);

        struct IOAPICDesc
        {
            ACPI::MADT_IOAPIC entry;
            uintptr_t virt;
            uint8_t version, maxRedirs;
            uint8_t id;
        };

        struct GSIEntry
        {
            size_t descIdx;
            uint32_t pin;
        };

        class APIC
        {
        public:
            APIC();
            APIC(ACPI::MADT* madt);
            bool Initialize(ACPI::MADT* madt);
            void SendEOI();
        private:
            uint32_t ReadLAPIC(uint32_t reg);
            void WriteLAPIC(uint32_t reg, uint32_t value);
            uint32_t ReadIOAPIC(int index, uint32_t reg);
            void WriteIOAPIC(int index, uint32_t reg, uint32_t value);
            uint64_t ReadIOAPIC64(int index, uint32_t reg);
            void WriteIOAPIC64(int index, uint32_t reg, uint64_t value);
            GSIEntry ResolveGSI(uint32_t gsi);
            uint64_t BuildPolarityTrigger(uint16_t flags);
            void InitializeIRQ(uint8_t irq);
            ACPI::MADT* madt;
            std::vector<ACPI::MADT_LAPIC> lapicEntries;
            std::vector<ACPI::MADT_X2APIC> x2ApicEntries;
            IOAPICDesc ioapicEntries[4];
            std::vector<ACPI::MADT_ISO> isoEntries;
            std::vector<GSIEntry> GSIs;
            ACPI::MADT_LAPIC_ADDR_OVERRIDE lapicOverride;
            bool X2APICSupported = false;
            uintptr_t lapicBaseVirt = 0;
            size_t ioapicEntryCount = 0;
            uint32_t irqToGSI[16];
        };
    }
}