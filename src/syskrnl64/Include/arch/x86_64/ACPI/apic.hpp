// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <arch/x86_64/ACPI/acpi.hpp>

#include <main/defs.hpp>

#include <const_array.hpp>
#include <vector>

#include <arch/x86_64/Interrupts/isr_mappings.hpp>

namespace krnl
{
	constexpr uint16_t APIC_MASTER_PIC_COMMAND_PORT = 0x20;
	constexpr uint16_t APIC_SLAVE_PIC_COMMAND_PORT = 0xA0;
	constexpr uint16_t APIC_MASTER_PIC_DATA_PORT = 0x21;
	constexpr uint16_t APIC_SLAVE_PIC_DATA_PORT = 0xA1;

	constexpr uint8_t APIC_MADT_LocalAPIC = 0;
	constexpr uint8_t APIC_MADT_IOAPIC = 1;
	constexpr uint8_t APIC_MADT_InterruptSourceOverride = 2;
	constexpr uint8_t APIC_MADT_NMI_Source = 3;
	constexpr uint8_t APIC_MADT_LocalAPIC_NMI = 4;
	constexpr uint8_t APIC_MADT_LocalAPIC_AddressOverride = 5;
	constexpr uint8_t APIC_MADT_IO_SAPIC = 6;
	constexpr uint8_t APIC_MADT_Local_SAPIC = 7;
	constexpr uint8_t APIC_MADT_PlatformInterruptSource = 8;
	constexpr uint8_t APIC_MADT_ProcessorLocalX2APIC = 9;
	constexpr uint8_t APIC_MADT_LocalX2APIC_NMI = 10;
	constexpr uint8_t APIC_MADT_GIC_CPU_Interface = 11;
	constexpr uint8_t APIC_MADT_GIC_Distributor = 12;
	constexpr uint8_t APIC_MADT_GIC_MSI_Frame = 13;
	constexpr uint8_t APIC_MADT_GIC_Redistrubutor = 14;
	constexpr uint8_t APIC_MADT_GIC_InterruptTranslationService = 15;
	constexpr uint8_t APIC_MADT_MultiprocessorWakeup = 16;

	constexpr uintptr_t APIC_IOREGSEL = 0x00 / 4;
	constexpr uintptr_t APIC_IOWIN = 0x10 / 4;

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

	constexpr uint32_t LAPIC_ICR_LOW = 0x300;
	constexpr uint32_t LAPIC_ICR_HIGH = 0x310;

	constexpr uint32_t LAPIC_ICR_DELIVERY_FIXED = 0b000 << 8;
	constexpr uint32_t LAPIC_ICR_DELIVERY_LOWEST = 0b001 << 8;
	constexpr uint32_t LAPIC_ICR_DELIVERY_SMI = 0b010 << 8;
	constexpr uint32_t LAPIC_ICR_DELIVERY_NMI = 0b100 << 8;
	constexpr uint32_t LAPIC_ICR_DELIVERY_INIT = 0b101 << 8;
	constexpr uint32_t LAPIC_ICR_DELIVERY_SIPI = 0b110 << 8;

	constexpr uint32_t LAPIC_ICR_DELIVERY_STATUS = 1 << 12;
	constexpr uint32_t LAPIC_ICR_LEVEL_ASSERT = 1 << 14;
	constexpr uint32_t LAPIC_ICR_TRIGGER_LEVEL = 1 << 15;

	struct IOAPIC_Desc
	{
		ACPI_MADT_IOAPIC entry;
		uintptr_t virt;
		uint8_t version, maxRedirs;
		uint8_t id;
	};

	struct APIC_GSIEntry
	{
		size_t descIdx;
		uint32_t pin;
	};

	struct APIC_CPUThreadDesc
	{
		uint32_t apicId;
		uint32_t flags;
		bool bsp;
	};

	enum IOAPIC_TriggerMode : uint8_t
	{
		// Edge or Level trigerred? Edge = 0, Level = 1
		IOAPIC_TRIGGER_EDGE = 0,
		IOAPIC_TRIGGER_LEVEL = (1 << 0),

		// High or Low polarity? High = 0, Low = 1
		IOAPIC_TRIGGER_HIGH = 0,
		IOAPIC_TRIGGER_LOW = (1 << 1),
	};

	class APIC
	{
	public:
		KRNL_STATUS Initialize(ACPI_MADT* madt);
		KRNL_STATUS InitializeCurrentLP();
		
		uint32_t ReadLAPIC(uint32_t reg);
		void WriteLAPIC(uint32_t reg, uint32_t value);
		void WriteLapicICR(uint32_t apicID, uint32_t lowValue);
		
		std::pair<size_t, uint8_t> AllocateGSI(uint8_t trigger);
		uint8_t AllocateSpecificGSI(uint8_t requestedGSI, uint8_t trigger);
		
		void SendEOI();
		inline std::vector<APIC_CPUThreadDesc> getCPUThreads() { return cpuThreads; }
		uint32_t GetCurrentAPICID();
	private:
		KRNL_STATUS ParseMADT();
		KRNL_STATUS InitializeIOAPIC();

		uint32_t ReadIOAPIC(int index, uint32_t reg);
		void WriteIOAPIC(int index, uint32_t reg, uint32_t value);
		uint64_t ReadIOAPIC64(int index, uint32_t reg);
		void WriteIOAPIC64(int index, uint32_t reg, uint64_t value);

		APIC_GSIEntry ResolveGSI(uint32_t gsi);
		uint64_t BuildPolarityTrigger(uint16_t flags);
		void InitializeIRQ(uint8_t irq);
		uint8_t AllocateISREntry();
		bool BindGSIToVector(uint8_t gsi, uint8_t vector, uint8_t trigger);

		ACPI_MADT* madt;

		std::vector<ACPI_MADT_LAPIC> lapicEntries;
		std::vector<ACPI_MADT_X2APIC> x2ApicEntries;
		IOAPIC_Desc ioapicEntries[4];
		std::vector<ACPI_MADT_ISO> isoEntries;

		std::vector<APIC_GSIEntry> GSIs;
		stdEx::const_array<bool> allocatedGSIs;
		bool allocatedIRQs[IRQ_COUNT];

		uintptr_t lapicBaseVirt = 0;
		size_t ioapicEntryCount = 0;
		uint32_t irqToGSI[16];
		
		bool X2APICSupported = false;

		std::vector<APIC_CPUThreadDesc> cpuThreads;
	};
}