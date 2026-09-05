// SPDX-License-Identifier: GPL-3.0-or-later

#include <arch/x86_64/PCI/PCIe.hpp>
#include <arch/x86_64/Utility/io.hpp>

#include <stdio.hpp>
#include <stdlib.hpp>
#include <string.hpp>

#include <main/utils.hpp>

using namespace krnl;

bool PCIe::Initialize(ACPI_MCFG* mcfg)
{
	if(!mcfg) {
		printf("[SYSKRNL64] [PCIe] [ERROR]: Invalid Initializer args\r\n");
		return false;
	}

	// Allocate and fill out a 256 entry array of ECAM Segments for each bus for quick ECAM Segment base lookup based off bus index
	
	this->Segments = reinterpret_cast<PCIe_ECAMSegment*>(kmalloc(sizeof(PCIe_ECAMSegment) * PCIE_MAX_BUSSES));
	if(!this->Segments)
	{
		printf("[SYSKRNL64] [PCIe] [ERROR]: Failed to allocate memory for storing ECAM Segment descriptors\r\n");
		return false;
	}

	size_t mcfgEntryCount = (mcfg->sdt.Length - sizeof(ACPI_SDTHeader) - 8) / sizeof(ACPI_MCFGEntry);

	for(size_t bus = 0; bus < PCIE_MAX_BUSSES; bus++)
	{
		ACPI_MCFGEntry* mcfgEntry = nullptr;
		bool found = false;
		for(size_t i = 0; i < mcfgEntryCount; i++) {
			mcfgEntry = &mcfg->entries[i];
			if(bus >= mcfgEntry->StartBus && bus <= mcfgEntry->EndBus) {
				found = true;
				break;
			}
		}

		if(!found) {
			this->BussesImplemented = bus;
			break;
		}
		this->Segments[bus].ECAMBase = mcfgEntry->BaseAddr + (bus - mcfgEntry->StartBus) * MEBIBYTE;
	}
	if(!this->BussesImplemented) this->BussesImplemented = 256;

	return true;
}

bool PCIe::GetDeviceInfo(uint8_t bus, uint8_t device, PCIe_DeviceInfo* infoOut)
{
	if(!infoOut || bus >= this->BussesImplemented || device >= PCIE_MAX_DEVICES) {
		printf("[SYSKRNL64] [PCIe] [ERROR]: Invalid args passed to GetDeviceInfo()\r\n");
		return false;
	}

	uintptr_t ConfigRegion = this->Segments[bus].ECAMBase;

	PCIe_ConfigBlock* configPhys = reinterpret_cast<PCIe_ConfigBlock*>(ConfigRegion + device * 32 * KIBIBYTE);

	// Map config block(4K) to virtual memory. The address is already page aligned

	auto allocRes = virtAlloc->AllocateBlocks(1, VA_NODE_FLAG_MMIO | VA_NODE_FLAG_NO_EXECUTE_ACCESS | VA_NODE_FLAG_USED);

	if(!allocRes) {
		printf("[SYSKRNL64] [PCIe] [ERROR]: Failed to allocate memory for PCIe Config Block, error code: %lu\r\n", allocRes.error());
		return false;
	}

	PCIe_ConfigBlock* config = reinterpret_cast<PCIe_ConfigBlock*>(allocRes.value()); 

	paging->MapArea(reinterpret_cast<uintptr_t>(configPhys), reinterpret_cast<uintptr_t>(config), 1, PTE_PRESENT | PTE_RW | PTE_PCD | PTE_NX);

	if(config->VendorID == PCIE_ANY16) return false; // Device doesn't exist

	// Fill out device info

	infoOut->Bus = bus;
	infoOut->Device = device;

	infoOut->VendorID = config->VendorID;
	infoOut->DeviceID = config->DeviceID;
	infoOut->progIF = config->ProgIF;
	infoOut->subClass = config->SubClass;
	infoOut->classCode = config->ClassCode;

	infoOut->root = config;

	infoOut->ExtraFunctions = config + 1;

	uint8_t headerType = config->HeaderType & 0x7F;
	uint8_t deviceBarCount = 0;

	switch(headerType)
	{
		case 0x00: deviceBarCount = 6; break; // Normal Device
		case 0x01: deviceBarCount = 2; break; // PCI-PCI Bridge
		case 0x02: deviceBarCount = 1; break; // Card Bus
		default: break;
	}
	
	for(size_t i = 0; i < deviceBarCount; i++)
	{
		infoOut->BARs[i].BarOffset = i * 4 + PCIE_BAR0;
		if(!GetBARInfo(infoOut, &infoOut->BARs[i])) {
			printf("[SYSKRNL64] [PCIe] [ERROR]: Failed to get BAR Info\r\n");
			return false;
		}

		if(infoOut->BARs[i].BarArch == PCIe_BARArchs::AMD64) i++;
	}

	return true;
}

bool PCIe::LocateDevice(PCIe_DeviceInfo* filters, PCIe_DeviceInfo* infoOut)
{
	if(!infoOut || !filters) {
		printf("[SYSKRNL64] [PCIe] [ERROR]: Invalid args passed to LocateDevice()\r\n");
		return false;
	}
	// Start enumerating from the bus/device specified in the filters to allow accessing multiple devices

	bool PCIBridgePresent = true;

	for(uint8_t bus = filters->Bus; bus < this->BussesImplemented && PCIBridgePresent; bus++) 
	{
		PCIBridgePresent = false;
		uint8_t devStart = (bus == filters->Bus) ? filters->Device : 0;
		for(uint8_t device = devStart; device < PCIE_MAX_DEVICES; device++)
		{
			// Existing devices root config space is always in function 0, other functions are only implemented for devices that need them

			if(!GetDeviceInfo(bus, device, infoOut)) continue; // Device in current slot doesn't exist, try next slot

			// Check if device is a PCI Bridge, and if yes, stop current search and start searching in the next bus

			if(FilterOut(&PCIBridgeFilter, infoOut)) {
				PCIBridgePresent = true;
				break; // Stops current loop and starts next one
			}

			// Device exists, compare with filters to check if the device is the correct one
			// If filter is PCIE_ANY16(0xFFFF) or PCIE_ANY8(0xFF), then that specific filter is nonexistent and that value can be any

			if(FilterOut(filters, infoOut)) return true;
		}
	}

	memset(infoOut, 0, sizeof(PCIe_DeviceInfo)); // Make sure that invalid information doesn't get passed by accident
	return false;
}

bool PCIe::FilterOut(const PCIe_DeviceInfo* filters, PCIe_DeviceInfo* cmp)
{
	if((cmp->VendorID == filters->VendorID || filters->VendorID == PCIE_ANY16) &&
	   (cmp->DeviceID == filters->DeviceID || filters->DeviceID == PCIE_ANY16) &&
	   (cmp->progIF == filters->progIF || filters->progIF == PCIE_ANY8) &&
	   (cmp->subClass == filters->subClass || filters->subClass == PCIE_ANY8) &&
	   (cmp->classCode == filters->classCode || filters->classCode == PCIE_ANY8)) return true;
	return false;            
}

bool PCIe::GetBARInfo(PCIe_DeviceInfo* device, PCIe_BARInfo* info, uint8_t function)
{
	if(!device || !info || info->BarOffset < PCIE_BAR0 || info->BarOffset > PCIE_BAR5) {
		printf("[SYSKRNL64] [PCIe] [ERROR]: Invalid args passed to GetBARInfo()\r\n");
		return false;
	}

	BAR* bar;
	uint8_t BarIndex = (info->BarOffset - PCIE_BAR0) / 4;
	if(function == 0) bar = &device->root->BARs[BarIndex];
	else {
		PCIe_ConfigBlock* config = &device->ExtraFunctions[function - 1];
		bar = &config->BARs[BarIndex];
	}

	// Get BAR Type, 1: I/O, 0: MMIO
	if(*bar & 0x1) {
		info->BarType = PCIe_BARTypes::IO;
		info->BarArch = PCIe_BARArchs::IA32;
		info->BarAddr = *bar & ~0x3;
		if(!info->BarAddr) return true;
	}
	else info->BarType = PCIe_BARTypes::MMIO;

	// Get BAR Arch, if type is I/O, always IA32, else if: 00: IA32, 10: AMD64, 01: invalid
	// Also get BAR Address, for I/O it's port index, for MMIO it's the physical address

	if(info->BarType == PCIe_BARTypes::MMIO) {
		uint32_t typeBits = (*bar >> 1) & 0b11;
		if(typeBits == 0b10) info->BarArch = PCIe_BARArchs::AMD64;
		else info->BarArch = PCIe_BARArchs::IA32;
		// Get BAR Address

		uint32_t AddrLow = *bar & ~0xF;
		uint32_t AddrHigh = 0;
		if(info->BarArch == PCIe_BARArchs::AMD64) {
			BAR upperBar;
			if(function == 0) upperBar = device->root->BARs[BarIndex + 1];
			else {
				PCIe_ConfigBlock* config = &device->ExtraFunctions[function - 1];
				upperBar = config->BARs[BarIndex + 1];
			}
			AddrHigh = upperBar;
		}

		info->BarAddr = ((uintptr_t)AddrHigh << 32) | AddrLow;

		if(!info->BarAddr) return true; // BAR Not present, return true to signify that the call didn't actually fail
	}

	// Save & modify command register

	uint16_t OrigCommand, *Command;
	if(function == 0) Command = &device->root->Command;
	else {
		PCIe_ConfigBlock* config = &device->ExtraFunctions[function - 1];
		Command = &config->Command;
	}
	OrigCommand = *Command;

	if(info->BarType == PCIe_BARTypes::IO) *Command &= ~PCIE_COMMAND_IO_SPACE;
	else if(info->BarType == PCIe_BARTypes::MMIO) *Command &= ~PCIE_COMMAND_MMIO_SPACE;

	// Write 0xFFFFFFFF to BAR to probe BAR Size

	if(info->BarArch == PCIe_BARArchs::IA32) {
		BAR orig = *bar;
		*bar = 0xFFFFFFFF;
		uint32_t BarMask = *bar;
		*bar = orig;

		if(info->BarType == PCIe_BARTypes::IO) info->BarLength = (~(BarMask & ~0x3) + 1);
		else if(info->BarType == PCIe_BARTypes::MMIO) info->BarLength = (~(BarMask & ~0xF) + 1);
	} else if(info->BarArch == PCIe_BARArchs::AMD64) {
		BAR orig1 = *bar;
		BAR* upperBar;
		if(function == 0) upperBar = &device->root->BARs[BarIndex + 1];
		else {
			PCIe_ConfigBlock* config = &device->ExtraFunctions[function - 1];
			upperBar = &config->BARs[BarIndex + 1];
		}
		BAR orig2 = *upperBar;
		*bar = 0xFFFFFFFF;
		*upperBar = 0xFFFFFFFF;
		uint32_t barMask = *bar;
		uint32_t upperBarMask = *upperBar;
		*bar = orig1;
		*upperBar = orig2;

		uint64_t fullMask = ((uint64_t)upperBarMask << 32) | (barMask & ~0xF);
		info->BarLength = (~fullMask + 1);
	}

	// Restore original command register

	*Command = OrigCommand;

	return true;
}