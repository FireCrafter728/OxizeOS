#pragma once
#include <stdint.h>
#include <stdbool.h>

#define PCI_MAX_BUSSES 256
#define PCI_MAX_DEVICES 32
#define PCI_MAX_FUNCTIONS 8

#define PCI_BAR0_OFFSET 0x10
#define PCI_BAR1_OFFSET 0x14
#define PCI_BAR2_OFFSET 0x18
#define PCI_BAR3_OFFSET 0x1C
#define PCI_BAR4_OFFSET 0x20
#define PCI_BAR5_OFFSET 0x24

#define PCI_ANY         0xFFFF
#define PCI_ANY_8b      0xFF

#define PCI_COMMAND_REG 0x04

namespace x86Kernel
{
    namespace PCI
    {
        struct DeviceInfo
        {
            uint16_t vendorID = PCI_ANY;
            uint16_t deviceID = PCI_ANY;
            uint8_t classCode = PCI_ANY_8b;
            uint8_t subclass = PCI_ANY_8b;
            uint8_t progIF = PCI_ANY_8b;
            uint8_t bus, device, function;
        };
        bool GetDeviceInfo(DeviceInfo *info, uint8_t bus, uint8_t device, uint8_t function);
        uint16_t ReadConfigWord(DeviceInfo *info, uint8_t offset);
        void WriteConfigWord(DeviceInfo *info, uint8_t offset, uint16_t value);
        uint32_t ReadBAR(DeviceInfo *info, uint8_t BarOffset);
        uint64_t ReadBAR64(DeviceInfo* info, uint8_t BarOffset);
        uint32_t GetBARSize(DeviceInfo* info, uint8_t BarOffset);
        bool IsBARIO(DeviceInfo *info, uint8_t BarOffset);
        bool FindDevice(DeviceInfo* infoIn, DeviceInfo* infoOut);
    };
}