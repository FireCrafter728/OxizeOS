#include <Drivers/PCI/pcie.hpp>

using namespace Bootloader;

ACPI::MCFG* mcfg;

bool PCIe::SetMCFG(ACPI::MCFG* mcfg)
{
	if (!mcfg) return false;
	::mcfg = mcfg;
	return true;
}

uintptr_t getECAMAddr(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset)
{
	if (!mcfg || !mcfg->entries) return 0;

	uint32_t entryCount = (mcfg->header.Length - sizeof(ACPI::ACPISDTHeader) - sizeof(uint64_t)) / sizeof(ACPI::MCFGEntry);

	for (uint32_t i = 0; i < entryCount; i++)
	{
		ACPI::MCFGEntry& e = mcfg->entries[i];

		if (bus >= e.startBus && bus <= e.endBus)
		{
			uintptr_t addr = (uintptr_t)e.baseAddr + ((uintptr_t)(bus - e.startBus) << 20) + ((uintptr_t)device << 15) + ((uintptr_t)function << 12) + (offset & ~3);
			return addr;
		}
	}

	return 0;
}

inline uint32_t Read32(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset)
{
    uintptr_t addr = getECAMAddr(bus, device, function, offset);
    if (!addr) return 0xFFFFFFFF; // invalid / no device
    return *(volatile uint32_t*)addr;
}

inline uint16_t Read16(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset)
{
    uintptr_t addr = getECAMAddr(bus, device, function, offset);
    if (!addr) return 0xFFFF;
    return *(volatile uint16_t*)addr;
}

inline uint8_t Read8(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset)
{
    uintptr_t addr = getECAMAddr(bus, device, function, offset);
    if (!addr) return 0xFF;
    return *(volatile uint8_t*)addr;
}

inline void Write32(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint32_t value)
{
    uintptr_t addr = getECAMAddr(bus, device, function, offset);
    if (!addr) return;
    *(volatile uint32_t*)addr = value;
}

inline void Write16(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint16_t value)
{
    uintptr_t addr = getECAMAddr(bus, device, function, offset);
    if (!addr) return;
    *(volatile uint16_t*)addr = value;
}

inline void Write8(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint8_t value)
{
    uintptr_t addr = getECAMAddr(bus, device, function, offset);
    if (!addr) return;
    *(volatile uint8_t*)addr = value;
}

bool PCIe::GetDeviceInfo(DeviceInfo* info, uint8_t bus, uint8_t device, uint8_t function)
{
	if (!info) return false;

    info->vendorId = Read16(bus, device, function, 0x00);
    if (info->vendorId == PCIE_ANY) return false;

    info->deviceId = Read16(bus, device, function, 0x02);
    info->classCode = Read8(bus, device, function, 0x0B);
    info->subClass = Read8(bus, device, function, 0x0A);
    info->progIF = Read8(bus, device, function, 0x09);

    info->bus = bus;
    info->device = device;
    info->function = function;

    return true;
}

uint16_t PCIe::ReadConfigWord(DeviceInfo* info, uint8_t offset)
{
    return Read16(info->bus, info->device, info->function, offset);
}

void PCIe::WriteConfigWord(DeviceInfo* info, uint8_t offset, uint16_t value)
{
    Write16(info->bus, info->device, info->function, offset, value);
}

uint64_t PCIe::ReadBAR(DeviceInfo* info, uint8_t BarOffset)
{
    uint32_t low = Read32(info->bus, info->device, info->function, BarOffset);
    if (!low || low == 0xFFFFFFFF) return 0;

    if (low & 0x1) return(uint16_t)(low & ~3);

    uint8_t type = (low >> 1) & 0x3;
    if (type == 0x2)
    {
        uint32_t high = Read32(info->bus, info->device, info->function, BarOffset + 4);
        uint64_t addr = ((uint64_t)high << 32) | (low & 0xFFFFFFF0);
        return addr;
    }
    else return (uint64_t)(low & 0xFFFFFFF0);
}

uint64_t PCIe::GetBARSize(DeviceInfo* info, uint8_t BarOffset)
{
    uint32_t originalLow = Read32(info->bus, info->device, info->function, BarOffset);
    if (!originalLow || originalLow == 0xFFFFFFFF) return 0;

    Write32(info->bus, info->device, info->function, BarOffset, 0xFFFFFFFF);
    uint32_t sizeLow = Read32(info->bus, info->device, info->function, BarOffset);
    Write32(info->bus, info->device, info->function, BarOffset, originalLow);

    if (originalLow & 1)
    {
        uint32_t mask = sizeLow & ~3U;
        uint32_t size = (~mask + 1);
        return (uint64_t)size;
    }

    uint8_t type = (originalLow >> 1) & 0x3;
    if (type == 0x2)
    {
        uint32_t originalHigh = Read32(info->bus, info->device, info->function, BarOffset + 4);
        Write32(info->bus, info->device, info->function, BarOffset + 4, 0xFFFFFFFF);
        uint32_t sizeHigh = Read32(info->bus, info->device, info->function, BarOffset + 4);

        Write32(info->bus, info->device, info->function, BarOffset + 4, originalHigh);

        uint64_t mask = ((uint64_t)sizeHigh << 32) | (sizeLow & 0xFFFFFFF0);
        uint64_t size = (~mask + 1);
        return size;
    }

    uint32_t mask = sizeLow & 0xFFFFFFF0;
    uint32_t size = (~mask + 1);
    return (uint64_t)size;
}

bool PCIe::isBARIO(DeviceInfo* info, uint8_t BarOffset)
{
    uint32_t BarAddr = Read32(info->bus, info->device, info->function, BarOffset);
    return (BarAddr & 0x1) != 0;
}

bool PCIe::FindDevice(DeviceInfo* infoIn, DeviceInfo* infoOut)
{
    for (uint16_t i = 0; i < PCIE_MAX_BUSSES; i++) {
        for (uint8_t j = 0; j < PCIE_MAX_DEVICES; j++) {
            for (uint8_t k = 0; k < PCIE_MAX_FUNCTIONS; k++) {
                if (!PCIe::GetDeviceInfo(infoOut, i, j, k)) continue;

                if ((infoIn->vendorId == PCIE_ANY || infoOut->vendorId == infoIn->vendorId) && (infoIn->deviceId == PCIE_ANY || infoOut->deviceId == infoIn->deviceId) &&
                    (infoIn->classCode == PCIE_ANY_8B || infoOut->classCode == infoIn->classCode) && (infoIn->subClass == PCIE_ANY_8B || infoOut->subClass == infoIn->subClass) && (infoIn->progIF == PCIE_ANY_8B || infoOut->progIF == infoIn->progIF)) return true;
            }
        }
    }

    return false;
}