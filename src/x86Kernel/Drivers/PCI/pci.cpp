#include <PCI/pci.hpp>
#include <io.h>
#include <stdio.h>

using namespace x86Kernel;

#define PCI_CONF_ADDR_PORT  0xCF8
#define PCI_CONF_DATA_PORT  0xCFC

uint32_t MakeAddr(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
    // printf("[PCI MAKEADDR] [INFO]: bus: %u, device: %u, func: %u, off: %u, addr: %lu\r\n", bus, device, function, offset, (uint32_t)((1U << 31) | ((uint32_t)bus << 16) | ((uint32_t)device << 11) | ((uint32_t)function << 8) | (offset & 0xFC)));
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

    // printf("addr: %lu, barlow: %lu", address, barLow);

    if ((barLow & 0x1) == 0) {
        // Memory BAR
        // Mask lower 4 bits to get base address
        return barLow & 0xFFFFFFF0;
    } else {
        // I/O BAR
        // Mask lower 2 bits to get base address
        return barLow & 0xFFFFFFFC;
    }

    return 0; // prevent compiler warn
}

uint64_t PCI::ReadBAR64(DeviceInfo* info, uint8_t BarOffset)
{
    uint32_t barLow, barHigh = 0;
    uint32_t address;

    // Make sure BarOffset is DWORD aligned
    address = MakeAddr(info->bus, info->device, info->function, BarOffset & 0xFC);

    outd(PCI_CONF_ADDR_PORT, address);
    barLow = ind(PCI_CONF_DATA_PORT);

    // printf("addr: %lu, barlow: %lu", address, barLow);

    if ((barLow & 0x1) == 0) {
        // Memory BAR
        // Mask lower 4 bits to get base address
        uint8_t type = (barLow >> 1) & 0x3;
        if(type == 0x2) {
            address = MakeAddr(info->bus, info->device, info->function, (BarOffset & 0xFC) + 4);
            outd(PCI_CONF_ADDR_PORT, address);
            barHigh = ind(PCI_CONF_DATA_PORT);
            // printf("addrHi: %lu, barHi: %lu", address, barHigh);
            return ((uint64_t)barHigh << 32) | (barLow & 0xFFFFFFF0);
        }
        return barLow & 0xFFFFFFF0;
    } else {
        // I/O BAR
        // Mask lower 2 bits to get base address
        return barLow & 0xFFFFFFFC;
    }

    return 0; // prevent compiler warn
}

uint32_t PCI::GetBARSize(DeviceInfo* info, uint8_t BarOffset)
{
    uint32_t addr = MakeAddr(info->bus, info->device, info->function, BarOffset & 0xFC);

    outd(PCI_CONF_ADDR_PORT, addr);
    uint32_t origBar = ind(PCI_CONF_DATA_PORT);

    outd(PCI_CONF_ADDR_PORT, addr);
    outd(PCI_CONF_DATA_PORT, 0xFFFFFFFF);

    outd(PCI_CONF_ADDR_PORT, addr);
    uint32_t sizeMask = ind(PCI_CONF_DATA_PORT);

    outd(PCI_CONF_ADDR_PORT, addr);
    outd(PCI_CONF_DATA_PORT, origBar);

    if((origBar & 0x1) == 0) sizeMask &= 0xFFFFFFF0;
    else sizeMask &= 0xFFFFFFFC;
    return (~sizeMask) + 1;
}

bool PCI::IsBARIO(DeviceInfo *info, uint8_t BarOffset)
{
    uint32_t barLow, address;

    address = MakeAddr(info->bus, info->device, info->function, BarOffset & 0xFC);

    outd(PCI_CONF_ADDR_PORT, address);
    barLow = ind(PCI_CONF_DATA_PORT);

    return (barLow & 0x1) != 0;
}

bool PCI::FindDevice(DeviceInfo* infoIn, DeviceInfo* infoOut)
{
    for (uint16_t i = 0; i < PCI_MAX_BUSSES; i++) {
        for (uint8_t j = 0; j < PCI_MAX_DEVICES; j++) {
            for (uint8_t k = 0; k < PCI_MAX_FUNCTIONS; k++) {
                if (!PCI::GetDeviceInfo(infoOut, i, j, k))
                    continue;

                if ((infoIn->vendorID == PCI_ANY || infoOut->vendorID == infoIn->vendorID) &&
                    (infoIn->deviceID == PCI_ANY || infoOut->deviceID == infoIn->deviceID) &&
                    (infoIn->classCode == PCI_ANY_8b || infoOut->classCode == infoIn->classCode) &&
                    (infoIn->subclass == PCI_ANY_8b || infoOut->subclass == infoIn->subclass) &&
                    (infoIn->progIF == PCI_ANY_8b || infoOut->progIF == infoIn->progIF))
                {
                    return true;
                }
            }
        }
    }
    return false;
}
