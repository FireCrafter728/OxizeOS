#include <Drivers/PCI/pci.hpp>

using namespace Bootloader;

#define PCI_CONF_ADDR_PORT 0xCF8
#define PCI_CONF_DATA_PORT 0xCFC

#define MakeAddr(bus, device, function, offset) ((uint32_t)((1U << 31) | ((uint32_t)bus << 16) | ((uint32_t)device << 11) | ((uint32_t)function << 8) | (offset & 0xFC)))

bool PCI::GetDeviceInfo(DeviceInfo* info, uint8_t bus, uint8_t device, uint8_t function)
{
	uint32_t addr = MakeAddr(bus, device, function, 0);
	outd(PCI_CONF_ADDR_PORT, addr);
	uint32_t data = ind(PCI_CONF_DATA_PORT);

	info->vendorId = data & 0xFFFF;
	if (info->vendorId == 0xFFFF) return false;

	info->deviceId = (data >> 16) & 0xFFFF;
	addr = MakeAddr(bus, device, function, 0x08);
	outd(PCI_CONF_ADDR_PORT, addr);
	data = ind(PCI_CONF_DATA_PORT);

	info->classCode = (data >> 24) & 0xFF;
	info->subClass = (data >> 16) & 0xFF;
	info->progIF = (data >> 8) & 0xFF;

	info->bus = bus;
	info->device = device;
	info->function = function;

	return true;
}

uint16_t PCI::ReadConfigWord(DeviceInfo* info, uint8_t offset)
{
	uint32_t addr = MakeAddr(info->bus, info->device, info->function, offset & 0xFC);
	outd(PCI_CONF_ADDR_PORT, addr);
	uint32_t data = ind(PCI_CONF_DATA_PORT);
	
	uint8_t shift = (offset & 2) * 8;
	return (data >> shift) & 0xFFFF;
}

void PCI::WriteConfigWord(DeviceInfo* info, uint8_t offset, uint16_t value)
{
	uint32_t addr = MakeAddr(info->bus, info->device, info->function, offset & 0xFC);
	outd(PCI_CONF_ADDR_PORT, addr);
	uint32_t data = ind(PCI_CONF_DATA_PORT);

	uint8_t shift = (offset & 2) * 8;
	data &= ~(0xFFFF << shift);
	data |= ((uint32_t)value << shift);

	outd(PCI_CONF_ADDR_PORT, addr);
	outd(PCI_CONF_DATA_PORT, data);
}

uint32_t PCI::ReadBAR(DeviceInfo* info, uint8_t BarOffset)
{
	uint32_t barLow, addr;

	addr = MakeAddr(info->bus, info->device, info->function, BarOffset & 0xFC);
	outd(PCI_CONF_ADDR_PORT, addr);
	barLow = ind(PCI_CONF_DATA_PORT);

	if ((barLow & 0x1) == 0) {
		return barLow & 0xFFFFFFF0;
	}
	else {
		return barLow & 0xFFFFFFFC;
	}

	return 0;
}

uint64_t PCI::ReadBAR64(DeviceInfo* info, uint8_t BarOffset)
{
	uint32_t barLow, barHigh = 0;
	uint32_t addr = MakeAddr(info->bus, info->device, info->function, BarOffset & 0xFC);

	outd(PCI_CONF_ADDR_PORT, addr);
	barLow = ind(PCI_CONF_DATA_PORT);

	if ((barLow & 0x1) == 0) {
		uint8_t type = (barLow >> 1) & 0x3;
		if (type == 0x2) {
			addr = MakeAddr(info->bus, info->device, info->function, (BarOffset & 0xFC) + 4);
			outd(PCI_CONF_ADDR_PORT, addr);
			barHigh = ind(PCI_CONF_DATA_PORT);
			return ((uint64_t)barHigh << 32) | (barLow & 0xFFFFFFF0);
		}
		return barLow & 0xFFFFFFF0;
	}
	else return barLow & 0xFFFFFFFC;

	return 0;
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

	if ((origBar & 0x1) == 0) sizeMask &= 0xFFFFFFF0;
	else sizeMask &= 0xFFFFFFFC;
	return (~sizeMask) + 1;
}

uint64_t PCI::GetBARSize64(DeviceInfo* info, uint8_t BarOffset) {
	uint32_t addr = MakeAddr(info->bus, info->device, info->function, BarOffset & 0xFC);
	outd(PCI_CONF_ADDR_PORT, addr);
	uint32_t origLow = ind(PCI_CONF_DATA_PORT);
	outd(PCI_CONF_ADDR_PORT, addr + 4);
	uint32_t origHigh = ind(PCI_CONF_DATA_PORT);

	outd(PCI_CONF_ADDR_PORT, addr);
	outd(PCI_CONF_DATA_PORT, 0xFFFFFFFF);
	outd(PCI_CONF_ADDR_PORT, addr + 4);
	outd(PCI_CONF_DATA_PORT, 0xFFFFFFFF);

	outd(PCI_CONF_ADDR_PORT, addr);
	uint32_t sizeLow = ind(PCI_CONF_DATA_PORT);
	outd(PCI_CONF_ADDR_PORT, addr + 4);
	uint32_t sizeHigh = ind(PCI_CONF_DATA_PORT);

	outd(PCI_CONF_ADDR_PORT, addr);
	outd(PCI_CONF_DATA_PORT, origLow);
	outd(PCI_CONF_ADDR_PORT, addr + 4);
	outd(PCI_CONF_DATA_PORT, origHigh);

	uint64_t sizeMask = ((uint64_t)sizeHigh << 32) | (sizeLow & 0xFFFFFFF0);
	return (~sizeMask) + 1;
}

bool PCI::isBARIO(DeviceInfo* info, uint8_t BarOffset)
{
	uint32_t barLow, addr;
	
	addr = MakeAddr(info->bus, info->device, info->function, BarOffset & 0xFC);
	outd(PCI_CONF_ADDR_PORT, addr);
	barLow = ind(PCI_CONF_DATA_PORT);

	return (barLow & 0x1) != 0;
}

bool PCI::FindDevice(DeviceInfo* infoIn, DeviceInfo* infoOut)
{
	for (uint16_t i = 0; i < PCI_MAX_BUSSES; i++) {
		for (uint8_t j = 0; j < PCI_MAX_DEVICES; j++) {
			for (uint8_t k = 0; k < PCI_MAX_FUNCTIONS; k++) {
				if (!PCI::GetDeviceInfo(infoOut, i, j, k)) continue;

				if ((infoIn->vendorId == PCI_ANY || infoOut->vendorId == infoIn->vendorId) && (infoIn->deviceId == PCI_ANY || infoOut->deviceId == infoIn->deviceId) &&
					(infoIn->classCode == PCI_ANY_8B || infoOut->classCode == infoIn->classCode) && (infoIn->subClass == PCI_ANY_8B || infoOut->subClass == infoIn->subClass) && (infoIn->progIF == PCI_ANY_8B || infoOut->progIF == infoIn->progIF)) return true;
			}
		}
	}

	return false;
}