// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>

#include <SysTable.hpp>
#include <expected>

namespace krnl
{
	// --------------- //
	// Core structures //
	// --------------- //

	struct PACK ACPI_RSDP
	{
		// RSDP
		uint8_t Signature[8]; // "RSD PTR "
		uint8_t Checksum;
		uint8_t OEMID[6];
		uint8_t Revision;
		uint32_t RsdtAddress;

		// XSDP Extension, in this case, ALWAYS present
		uint32_t Length;
		uint64_t XsdtAddress;
		uint8_t ExtendedChecksum;
		uint8_t reserved[3];
	};

	struct PACK ACPI_SDTHeader
	{
		uint8_t Signature[4];
		uint32_t Length;
		uint8_t Revision;
		uint8_t Checksum;
		uint8_t OEMID[6];
		uint8_t OEMTableID[8];
		uint32_t OEMRevision;
		uint32_t CreatorID;
		uint32_t CreatorRevision;
	};

	struct PACK ACPI_XSDT
	{
		ACPI_SDTHeader sdt;
		uintptr_t entries[];
	};

	// -------------------- //
	// PCIe MCFG Structures //
	// -------------------- //

	struct PACK ACPI_MCFGEntry
	{
		uint64_t BaseAddr;
		uint16_t SegmentGroup;
		uint8_t StartBus;
		uint8_t EndBus;
		uint32_t Reserved;
	};

	struct PACK ACPI_MCFG
	{
		ACPI_SDTHeader sdt;
		uint64_t Reserved;
		ACPI_MCFGEntry entries[];
	};

	// -------------------- //
	// APIC MADT Structures //
	// -------------------- //

	struct PACK ACPI_MADT
	{
		ACPI_SDTHeader sdt;
		uint32_t lapicAddr;
		uint32_t flags;
	};

	struct PACK ACPI_MADTEntryHeader
	{
		uint8_t type, length;   
	};  

	struct PACK ACPI_MADT_LAPIC
	{
		ACPI_MADTEntryHeader hdr;
		uint8_t apiccpuid, apicid;
		uint32_t flags;
	};

	struct PACK ACPI_MADT_IOAPIC
	{
		ACPI_MADTEntryHeader hdr;
		uint8_t ioapicId, reserved;
		uint32_t ioapicAddr;
		uint32_t globalSysInterruptBase;
	};

	struct PACK ACPI_MADT_ISO
	{
		ACPI_MADTEntryHeader hdr;
		uint8_t busSource, IRQSource;
		uint32_t globalSysIntr;
		uint16_t flags;
	};

	struct PACK ACPI_MADT_IOAPIC_NMI
	{
		ACPI_MADTEntryHeader hdr;
		uint8_t nmiSource, reserved;
		uint16_t flags;
		uint32_t gsi;
	};

	struct PACK ACPI_MADT_LAPIC_NMI
	{
		ACPI_MADTEntryHeader hdr;
		uint8_t acpicpuid;
		uint16_t flags;
		uint8_t lint;
	};

	struct PACK ACPI_MADT_LAPIC_ADDR_OVERRIDE
	{
		ACPI_MADTEntryHeader hdr;
		uint16_t reserved;
		uint64_t lapicAddr;
	};

	struct PACK ACPI_MADT_X2APIC
	{
		ACPI_MADTEntryHeader hdr;
		uint16_t reserved;
		uint32_t x2apicId, flags, apicProcID;
	};

	struct PACK ACPI_MADT_X2APIC_NMI
	{
		ACPI_MADTEntryHeader hdr;
		uint16_t flags;
		uint32_t apicProcID;
		uint8_t lint, reserved[3];
	};

	// --------------- //
	// HPET Structures //
	// --------------- //

	struct PACK ACPI_HPET_Address
	{
		uint8_t addressSpaceID; // 0: system memory, 1: system I/O
		uint8_t registerBitWidth;
		uint8_t registerBitOffset;
		uint8_t _Reserved;
		uintptr_t address;
	};

	struct PACK ACPI_HPET_Attributes
	{   
		uint8_t comparatorCount : 5;
		uint8_t counterSize : 1;
		uint8_t reserved : 1;
		uint8_t legacyReplacement : 1;
	};

	struct PACK ACPI_HPET
	{
		ACPI_SDTHeader hdr;
		uint8_t hardwareRevisionID;
		ACPI_HPET_Attributes attributes;
		uint16_t vendorID;
		ACPI_HPET_Address address;
		uint8_t hpetNumber;
		uint16_t minimumTick;
		uint8_t pageProtection;
	};

	class ACPI
	{
	public:
		ACPI() = default;
		ACPI(SystemTable* System);
		bool Initialize(SystemTable* System);
		ACPI_MCFG* GetMCFG();
		ACPI_MADT* GetMADT();
		ACPI_HPET* GetHPET();
	private:
		ACPI_SDTHeader* GetMappedStructure(uint32_t signature);
		ACPI_RSDP* rsdp;
		ACPI_XSDT* xsdt;
		size_t xsdtEntries;
		ACPI_MCFG* mcfg;
		ACPI_MADT* madt;
		ACPI_HPET* hpet;
		static constexpr char RSDPSignature[8] = {'R', 'S', 'D', ' ', 'P', 'T', 'R', ' '};
		static constexpr char XSDTSignature[4] = {'X', 'S', 'D', 'T'};
		static constexpr uint32_t MCFGSignature = 0x4746434D;
		static constexpr uint32_t MADTSignature = 0x43495041;
		static constexpr uint32_t HPETSignature = 0x54455048;
	};
}