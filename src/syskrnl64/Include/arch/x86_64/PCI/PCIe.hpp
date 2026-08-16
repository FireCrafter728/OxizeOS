// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>
#include <arch/x86_64/std/stdint.hpp>

#include <arch/x86_64/ACPI/acpi.hpp>

namespace krnl
{
	typedef uint32_t BAR;

	constexpr uint16_t PCIE_MAX_BUSSES = 256;
	constexpr uint8_t PCIE_MAX_DEVICES = 32;
	constexpr uint8_t PCIE_MAX_FUNCTIONS = 8;
	
	constexpr uint16_t PCIE_ANY16 = 0xFFFF;
	constexpr uint8_t PCIE_ANY8 = 0xFF;
	
	constexpr uint8_t PCIE_BAR0 = 0x10;
	constexpr uint8_t PCIE_BAR1 = 0x14;
	constexpr uint8_t PCIE_BAR2 = 0x18;
	constexpr uint8_t PCIE_BAR3 = 0x1C;
	constexpr uint8_t PCIE_BAR4 = 0x20;
	constexpr uint8_t PCIE_BAR5 = 0x24;
	
	constexpr uint16_t PCIE_COMMAND_IO_SPACE = 0x01;
	constexpr uint16_t PCIE_COMMAND_MMIO_SPACE = 0x02;
	constexpr uint16_t PCIE_COMMAND_BUS_MASTER = 0x04;
	constexpr uint16_t PCIE_COMMAND_SPECIAL_CYCLES = 0x08;
	constexpr uint16_t PCIE_COMMAND_MEM_WRITE_INV = 0x10;
	constexpr uint16_t PCIE_COMMAND_VGA_PALLETE = 0x20;
	constexpr uint16_t PCIE_COMMAND_PARITY_ERROR = 0x40;
	constexpr uint16_t PCIE_COMMAND_WAIT_CYCLE = 0x80;
	constexpr uint16_t PCIE_COMMAND_SERR_ENABLE = 0x100;
	constexpr uint16_t PCIE_COMMAND_FAST_B2B = 0x200;

	constexpr uint16_t PCIE_CONFIG_COMMAND_IOEnable = (1U << 0);
	constexpr uint16_t PCIE_CONFIG_COMMAND_MMIOEnable = (1U << 1);
	constexpr uint16_t PCIE_CONFIG_COMMAND_BusMasterEnable = (1U << 2);
	constexpr uint16_t PCIE_CONFIG_COMMAND_InterruptDisable = (1U << 10);

	struct PACK PCIe_ConfigBlock
	{
		uint16_t VendorID, DeviceID;
		uint16_t Command, Status;
		uint8_t RevisionID;
		uint8_t ProgIF, SubClass, ClassCode;
		uint8_t CacheLineSize, LatencyTimer, HeaderType, BIST;

		BAR BARs[6];

		uint32_t CardbusCISPtr;
		uint16_t SubsystemVendorID, SubsystemID;
		uint32_t ExpansionROMBase;

		uint8_t CapPointer;

		uint8_t Reserved[7];
		uint64_t Reserved2;

		uint8_t Extended[0xFC0];
	};

	struct PACK PCIe_ECAMSegment
	{
		uintptr_t ECAMBase;
	};

	enum class PCIe_BARTypes : uint8_t
	{
		NONE = 0,
		IO,
		MMIO,
	};

	enum class PCIe_BARArchs : uint8_t
	{
		IA32,
		AMD64,
	};

	struct PCIe_BARInfo
	{
		uint8_t BarOffset;
		uintptr_t BarAddr;
		PCIe_BARTypes BarType;
		PCIe_BARArchs BarArch;
		size_t BarLength;
	};

	struct PCIe_DeviceInfo
	{
		PCIe_ConfigBlock* root;
		PCIe_ConfigBlock* ExtraFunctions;
		uint8_t Bus, Device;
		uint16_t VendorID, DeviceID; // Duplication for simplicity and faster access, must mirror the actual data inside config block
		uint8_t progIF, subClass, classCode;
		PCIe_BARInfo BARs[6];
	};

	class PCIe
	{
	public:
		bool Initialize(ACPI_MCFG* mcfg);
		bool LocateDevice(PCIe_DeviceInfo* filters, PCIe_DeviceInfo* infoOut);
		bool GetDeviceInfo(uint8_t bus, uint8_t device, PCIe_DeviceInfo* infoOut);
	private:
		bool GetBARInfo(PCIe_DeviceInfo* device, PCIe_BARInfo* info, uint8_t function = 0);
		bool FilterOut(const PCIe_DeviceInfo* filters, PCIe_DeviceInfo* cmp);
		ACPI_MCFG* mcfg;
		PCIe_ECAMSegment* Segments;
		size_t BussesImplemented;
		static constexpr PCIe_DeviceInfo PCIBridgeFilter = {nullptr, nullptr, 0, 0, PCIE_ANY16, PCIE_ANY16, 0x00, 0x04, 0x06, {}};
	};
}