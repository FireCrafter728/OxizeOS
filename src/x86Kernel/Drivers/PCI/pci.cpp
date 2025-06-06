#include <PCI/pci.hpp>
#include <io.h>

using namespace x86Kernel;

#define PCI_CONF_ADDR_PORT  0xCF8
#define PCI_CONF_DATA_PORT  0xCFC

uint32_t MakeAddr(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
    return (uint32_t)((1U << 31) | ((uint32_t)bus << 16) | ((uint32_t)device << 11) | ((uint32_t)function << 8) | (offset & 0xFC));
}

bool PCI::GetDeviceInfo(DeviceInfo* info, uint8_t bus, uint8_t device, uint8_t function)
{
    uint32_t address = MakeAddr(bus, device, function, 0);
    outd(PCI_CONF_ADDR_PORT, address);
    uint32_t data = ind(PCI_CONF_DATA_PORT);

    info->vendorID = data & 0xFFFF;
    if(info->vendorID == 0xFFFF) return false;

    info->deviceID = (data >> 16) & 0xFFFF;
    address = MakeAddr(bus, device, function, 0x08);
    outd(PCI_CONF_ADDR_PORT, address);
    data = ind(PCI_CONF_DATA_PORT);

    info->classCode = (data >> 24) & 0xFF;
    info->subclass = (data >> 16) & 0xFF;
    info->progIF = (data >> 8) & 0xFF;

    info->bus = bus;
    info->device = device;
    info->function = function;

    return true;
}

uint16_t PCI::ReadConfigWord(DeviceInfo* info, uint8_t offset)
{
    uint32_t Address = MakeAddr(info->bus, info->device, info->function, offset & 0xFC);
    outd(PCI_CONF_ADDR_PORT, Address);
    uint32_t data = ind(PCI_CONF_DATA_PORT);

    uint8_t shift = (offset & 2) * 8;
    return (data >> shift) & 0xFFFF;
}

void PCI::WriteConfigWord(DeviceInfo* info, uint8_t offset, uint16_t value)
{
    uint32_t Address = MakeAddr(info->bus, info->device, info->function, offset & 0xFC);
    outd(PCI_CONF_ADDR_PORT, Address);
    uint32_t data = ind(PCI_CONF_DATA_PORT);

    uint8_t shift = (offset & 2) * 8;
    data &= ~(0xFFFF << shift);
    data |= ((uint32_t)value << shift);

    outd(PCI_CONF_ADDR_PORT, Address);
    outd(PCI_CONF_DATA_PORT, data);
}

uint32_t PCI::ReadBAR(DeviceInfo* info, uint8_t BarOffset)
{
    uint32_t barLow, address;

    // Make sure BarOffset is DWORD aligned
    address = MakeAddr(info->bus, info->device, info->function, BarOffset & 0xFC);

    outd(PCI_CONF_ADDR_PORT, address);
    barLow = ind(PCI_CONF_DATA_PORT);

    if ((barLow & 0x1) == 0) {
        // Memory BAR
        // Mask lower 4 bits to get base address
        return barLow & 0xFFFFFFF0;
    } else {
        // I/O BAR
        // Mask lower 2 bits to get base address
        return barLow & 0xFFFFFFFC;
    }

    return 0;
}