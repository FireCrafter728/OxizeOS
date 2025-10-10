#include <print.hpp>
#include <Drivers/ACPI/acpi.hpp>
#include <Drivers/PCI/pci.hpp>
#include <Drivers/PCI/pcie.hpp>

print Print;

EFI_SYSTEM_TABLE* gSystem;

bool TestPCI_PCIe()
{
	bool fail = false;
	const uint8_t BarOffsets[6] = { 0x10,0x14,0x18,0x1C,0x20,0x24 };

	Bootloader::PCI::DeviceInfo filter = {};
	filter.vendorId = PCI_ANY;
	filter.deviceId = PCI_ANY;
	filter.classCode = 0x01; // Mass storage Controller
	filter.subClass = 0x06; // SATA Controller
	filter.progIF = 0x01; // AHCI Programming interface

	Bootloader::PCI::DeviceInfo pciDevice = {};
	if (!Bootloader::PCI::FindDevice(&filter, &pciDevice)) {
		printf("Failed to find SATA AHCI Device in PCI\r\n");
		fail = true;
		goto PCIe_Test;
	}

	printf("Found PCI SATA AHCI Device at %x:%x.%x vendor=%x, deviceId=%x\r\n", pciDevice.bus, pciDevice.device, pciDevice.function, pciDevice.vendorId, pciDevice.deviceId);

	for (int i = 0; i < 6; i++)
	{
		uint8_t off = BarOffsets[i];
		bool isIO = Bootloader::PCI::isBARIO(&pciDevice, off);
		uint32_t addr = Bootloader::PCI::ReadBAR(&pciDevice, off);
		uint32_t size = Bootloader::PCI::GetBARSize(&pciDevice, off);

		if (addr == 0) continue;

		printf("PCI BAR%d: %s addr=%lx size=%lx\r\n", i, isIO ? "I/O" : "MMIO", addr, size);
	}

PCIe_Test:
	Bootloader::PCIe::DeviceInfo filterPCIe = {};
	filterPCIe.vendorId = PCI_ANY;
	filterPCIe.deviceId = PCI_ANY;
	filterPCIe.classCode = 0x01; // Mass storage Controller
	filterPCIe.subClass = 0x06; // SATA Controller
	filterPCIe.progIF = 0x01; // AHCI Programming interface

	Bootloader::PCIe::DeviceInfo pcieDevice = {};
	if (!Bootloader::PCIe::FindDevice(&filterPCIe, &pcieDevice)) {
		printf("Failed to find SATA AHCI Device in PCI\r\n");
		return false;
	}

	printf("Found PCIe SATA AHCI Device at %x:%x.%x vendor=%x, deviceId=%x\r\n", pcieDevice.bus, pcieDevice.device, pcieDevice.function, pcieDevice.vendorId, pcieDevice.deviceId);

	for (int i = 0; i < 6; i++)
	{
		uint8_t off = BarOffsets[i];
		bool isIO = Bootloader::PCIe::isBARIO(&pcieDevice, off);
		uint64_t addr = Bootloader::PCIe::ReadBAR(&pcieDevice, off);
		uint64_t size = Bootloader::PCIe::GetBARSize(&pcieDevice, off);

		if (addr == 0) continue;

		printf("PCIe BAR%d: %s addr=%llx size=%llx\r\n", i, isIO ? "I/O" : "MMIO", addr, size);
	}

	return !fail;
}

extern "C"
EFI_STATUS
EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* System)
{
	gSystem = System;
	Print.SetStdout(System->ConOut);
	Print.ClrScr();
	printf("OxizeOS Bootloader V0.1.0004\r\n");

	Bootloader::ACPI::ACPIDesc* acpiDesc = new Bootloader::ACPI::ACPIDesc;
	if (!acpiDesc) {
		printf("[BOOTLOADER] [MAIN] [ERROR]: Memory allocation failed!\r\n");
		HaltSystem();
	}
	Bootloader::ACPI::ACPI acpi(acpiDesc, System->ConfigurationTable, System->NumberOfTableEntries);

	Bootloader::ACPI::MCFG* mcfg = new Bootloader::ACPI::MCFG;

	if (!mcfg) {
		printf("[BOOTLOADER] [MAIN] [ERROR]: Memory allocation failed!\r\n");
		HaltSystem();
	}

	if (!acpi.GetMCFG(mcfg)) {
		printf("[BOOTLOADER] [MAIN] [ERROR]: Failed to get MCFG!\r\n");
		HaltSystem();
	}

	Bootloader::PCIe::SetMCFG(mcfg);

	TestPCI_PCIe();

	EFI_INPUT_KEY Key;
	while (System->ConIn->ReadKeyStroke(System->ConIn, &Key) != EFI_SUCCESS);

	delete acpiDesc;
	delete mcfg;
	
	System->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

	return EFI_SUCCESS;
}