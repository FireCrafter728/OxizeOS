#pragma once
#include <Global.hpp>

#define PCI_MAX_BUSSES 256
#define PCI_MAX_DEVICES 24
#define PCI_MAX_FUNCTIONS 8

#define PCI_BAR0_OFFSET 0x10
#define PCI_BAR1_OFFSET 0x14
#define PCI_BAR2_OFFSET 0x18
#define PCI_BAR3_OFFSET 0x1C
#define PCI_BAR4_OFFSET 0x20
#define PCI_BAR5_OFFSET 0x24

#define PCI_ANY 0xFFFF
#define PCI_ANY_8B 0xFF

#define PCI_COMMAND_REG 0x04

namespace Bootloader
{
	namespace PCI
	{
		struct DeviceInfo
		{
			uint16_t vendorId = PCI_ANY, deviceId = PCI_ANY;
			uint8_t classCode = PCI_ANY_8B, subClass = PCI_ANY_8B, progIF = PCI_ANY_8B;
			uint8_t bus, device, function;
		};

		bool GetDeviceInfo(DeviceInfo* info, uint8_t bus, uint8_t device, uint8_t function);
		uint16_t ReadConfigWord(DeviceInfo* info, uint8_t offset);
		void WriteConfigWord(DeviceInfo* info, uint8_t offset, uint16_t value);
		uint32_t ReadBAR(DeviceInfo* info, uint8_t BarOffset);
		uint64_t ReadBAR64(DeviceInfo* info, uint8_t BarOffset);
		uint32_t GetBARSize(DeviceInfo* info, uint8_t BarOffset);
		uint64_t GetBARSize64(DeviceInfo* info, uint8_t BarOffset);
		bool isBARIO(DeviceInfo* info, uint8_t BarOffset);
		bool FindDevice(DeviceInfo* infoIn, DeviceInfo* infoOut);
	}
}