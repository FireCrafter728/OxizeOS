#pragma once
#include <Drivers/ACPI/acpi.hpp>

#define PCIE_MAX_BUSSES 256
#define PCIE_MAX_DEVICES 24
#define PCIE_MAX_FUNCTIONS 8

#define PCIE_BAR0_OFFSET 0x10
#define PCIE_BAR1_OFFSET 0x14
#define PCIE_BAR2_OFFSET 0x18
#define PCIE_BAR3_OFFSET 0x1C
#define PCIE_BAR4_OFFSET 0x20
#define PCIE_BAR5_OFFSET 0x24

#define PCIE_ANY 0xFFFF
#define PCIE_ANY_8B 0xFF

#define PCIE_COMMAND_REG 0x04
#define PCIE_STATUS_REG 0x06

namespace Bootloader
{
	namespace PCIe
	{
		struct DeviceInfo
		{
			uint16_t vendorId = PCIE_ANY, deviceId = PCIE_ANY;
			uint8_t classCode = PCIE_ANY_8B, subClass = PCIE_ANY_8B, progIF = PCIE_ANY_8B;
			uint8_t bus, device, function;
		};
		bool SetMCFG(ACPI::MCFG* mcfg);
		bool GetDeviceInfo(DeviceInfo* info, uint8_t bus, uint8_t device, uint8_t function);
		uint16_t ReadConfigWord(DeviceInfo* info, uint8_t offset);
		void WriteConfigWord(DeviceInfo* info, uint8_t offset, uint16_t value);
		uint64_t ReadBAR(DeviceInfo* info, uint8_t BarOffset);
		uint64_t GetBARSize(DeviceInfo* info, uint8_t BarOffset);
		bool isBARIO(DeviceInfo* info, uint8_t BarOffset);
		bool FindDevice(DeviceInfo* infoIn, DeviceInfo* infoOut);
	}
}