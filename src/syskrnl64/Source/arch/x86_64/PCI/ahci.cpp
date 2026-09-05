// SPDX-License-Identifier: GPL-3.0-or-later

#include <arch/x86_64/PCI/ahci.hpp>
#include <arch/x86_64/Utility/io.hpp>

#include <stdio.hpp>
#include <string.hpp>

#include <main/utils.hpp>

using namespace krnl;

constexpr size_t hbaBlocks = BLOCK_COUNT(sizeof(AHCI_HBA));

bool AHCI::Initialize(AHCIDevice* device)
{
	if(!device || !device->deviceInfo) {
		printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Invalid initializer args\r\n");
		return false;
	}

	// Retrieve PCIe BAR 5 and confirm it's integrity
	
	PCIe_BARInfo bar = device->deviceInfo->BARs[5];

	if(bar.BarType != PCIe_BARTypes::MMIO) {
		printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: AHCI BAR 5 is an I/O BAR, not an MMIO BAR\r\n");
		return false;
	}

	// Map PCIe BAR 5 to virtual memory

	auto barMapRes = virtAlloc->AllocateBlocks(hbaBlocks, VA_NODE_FLAG_MMIO | VA_NODE_FLAG_NO_EXECUTE_ACCESS | VA_NODE_FLAG_USED);
	
	if(!barMapRes) {
		printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to reserve virtual memory for HBA, error code: %lu\r\n", barMapRes.error());
		return false;
	}

	device->hba = reinterpret_cast<AHCI_HBA*>(barMapRes.value());

	paging->MapArea(bar.BarAddr, reinterpret_cast<uintptr_t>(device->hba), hbaBlocks, PTE_PRESENT | PTE_RW | PTE_PCD | PTE_NX);

	//
	// Initializing AHCI
	//

	// Step 1: Enable Memory Space & Bus Master bits in PCIe Config Space command register
	device->deviceInfo->root->Command |= PCIE_CONFIG_COMMAND_MMIOEnable | PCIE_CONFIG_COMMAND_BusMasterEnable;

	// Step 2: Reset all ports to clean up after UEFI

	if(!ResetPorts(device)) {
		printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to reset ports\r\n");
		return false;
	}

	// Step 3: Reset the entire HBA

	device->hba->GlobalControl |= AHCI_GHC_HBAReset;

	int timeout = 100000;
	while(timeout-- && (device->hba->GlobalControl & AHCI_GHC_HBAReset) != 0);
	if(timeout <= 0) {
		printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to reset AHCI GHC\r\n");
		return false;
	}

	// Step 4: Enable AHCI In HBA

	device->hba->GlobalControl |= AHCI_GHC_AHCIEnable;
	if((device->hba->GlobalControl &AHCI_GHC_AHCIEnable) == 0) {
		printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to enable AHCI mode\r\n");
		return false;
	}

	// Step 5: Initialize all ports

	if(!InitializePorts(device)) {
		printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to initialize ports\r\n");
		return false;
	}

	// Step 6: Send IDENTIFY DEVICE for each port

	for(uint8_t i = 0; i < 32; i++)
	{
		if(!(device->hba->PortsImplemented & (1U << i))) continue;

		AHCI_PortDesc* desc = &device->ports[i];

		volatile AHCI_Port* port = desc->port;

		// Make sure port is implemented and active
		if((port->PxSSTS & 0x0F) != 3 || ((port->PxSSTS >> 8) & 0x0F) != 1) continue;

		int slot = -1;
		for(int j = 0; j < 32; j++)
		{
			if(!(port->PxSACT & (1U << j)) && !(port->PxCI & (1U << j))) {
				slot = j;
				break;
			}
		}

		if(slot == -1) {
			printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to find available slots for IDENTIFY DEVICE command for port %d\r\n", i);
			return false;
		}

		// Make sure port is ready for a command

		while(port->PxTFD & AHCI_PxTFD_Busy);
		while(port->PxTFD & AHCI_PxTFD_DRQ);

		// Get command header & table

		volatile AHCI_HBACommandHeader* commandHeader = &desc->CLB[slot];
		AHCI_CommandTable* cmdTable = desc->commandTables[slot];

		// Setup command header

		commandHeader->PRDTLength = 1;
		commandHeader->WriteDirection = 0;
		commandHeader->CommandFISLength = sizeof(AHCI_H2DFIS) / 4;

		// Setup command table

		volatile AHCI_PRDTEntry* entry0 = &cmdTable->PRDT[0];

		uintptr_t dataOutPhys = paging->GetPhys(reinterpret_cast<uintptr_t>(desc->IdentifyBuffer));

		entry0->DataBaseAddress = (uint32_t)(dataOutPhys & 0xFFFFFFFF);
		entry0->DataBaseAddressUpper = (uint32_t)(dataOutPhys >> 32);
		entry0->IOC_ByteCount = 511;

		volatile AHCI_H2DFIS* cfis = reinterpret_cast<volatile AHCI_H2DFIS*>(cmdTable->CFIS);

		memset((void*)cmdTable->CFIS, 0, 64);

		cfis->FISType = AHCI_FIS_TYPE_H2D;
		cfis->c = 1;
		cfis->command = AHCI_FIS_COMMAND_IDENTIFY;
		cfis->device = 0;

		port->PxIS = 0xFFFFFFFF;
		port->PxCI |= (1U << slot);

		// Command sent, wait for response

		int timeout = 1000000;
		while((port->PxCI & (1U << slot)) && timeout--);
		if(timeout <= 0)
		{
			printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to send IDENTIFY DEVICE for port %d, timeout\r\n", i);
			return false;
		}

		if(port->PxSERR != 0) {
			printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to send IDENTIFY DEVICE for port %d, PxSERR: 0x%lX\r\n", i, port->PxSERR);
			return false;
		}

		if(port->PxIS & AHCI_PxIS_TaskFileErrorStatus) {
			printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to send IDENTIFY DEVICE for port %d, Task File Error\r\n", i);
			return false;
		}

		if((port->PxTFD & 0xFF) & AHCI_PxTFD_Error) {
			printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to send IDENTIFY DEVICE for port %d, Status: 0x%lX, Error: 0x%lX\r\n", i, (port->PxTFD & 0xFF), ((port->PxTFD >> 8) & 0xFF));
			return false;
		}
	}

	return true;
}

bool AHCI::ResetPorts(AHCIDevice* device)
{
	for(uint8_t i = 0; i < 32; i++)
	{
		if(!(device->hba->PortsImplemented & (1U << i))) continue;

		volatile AHCI_Port* port = &device->hba->Ports[i];

		port->PxCMD &= ~(AHCI_PxCMD_Start | AHCI_PxCMD_FISReceiveEnable);

		while(port->PxCMD & (AHCI_PxCMD_CommandListRunning | AHCI_PxCMD_FISReceiveRunning));

		port->PxSERR = 0xFFFFFFFF;
		port->PxIS = 0xFFFFFFFF;

		port->PxCI = 0;
		port->PxSACT = 0;

		port->PxSCTL = (port->PxSCTL & ~0xF) | 1;

		for(int d = 0; d < 100000; d++);

		port->PxSCTL = (port->PxSCTL & ~0xF) | 3;

		int timeout = 100000;
		while(timeout--)
		{
			uint8_t det = port->PxSSTS & 0x0F;
			uint8_t ipm = (port->PxSSTS >> 8) & 0x0F;

			if(det == 3 && ipm == 1) break;
		}

		if(timeout <= 0) {
			printf("[SYSKRNL64] [SATA-AHCI] [WARN]: Port %d is implemented but unresponsive\r\n", i);
			continue;   
		}
	}

	return true;
}

bool AHCI::InitializePorts(AHCIDevice* device)
{
	for(uint8_t i = 0; i < 32; i++)
	{
		if(!(device->hba->PortsImplemented & (1U << i))) continue;

		AHCI_PortDesc& desc = device->ports[i];

		desc.port = &device->hba->Ports[i];

		volatile AHCI_Port* port = desc.port;

		if((port->PxSSTS & 0x0F) != 3 || ((port->PxSSTS >> 8) & 0x0F) != 1) continue; // Port not present or active, go to the next one

		// Setup CLB & FB

		auto cmdHeaderAllocRes = virtAlloc->AllocateBlocks(1, VA_NODE_FLAG_PHYSICALLY_NOT_BACKED | VA_NODE_FLAG_MMIO | VA_NODE_FLAG_NO_EXECUTE_ACCESS | VA_NODE_FLAG_USED);
		if(!cmdHeaderAllocRes) {
			printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to allocate virtual memory for port %d CLB, error code: %lu\r\n", i, cmdHeaderAllocRes.error());
			return false;
		}
		auto cmdHeaderPhysAllocRes = physAlloc->AllocContiguousBlocks(1);
		if(!cmdHeaderPhysAllocRes)
		{
			printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to allocate physical memory for port %d CLB, error code: %lu\r\n", i, cmdHeaderPhysAllocRes.error());
			return false;
		}
		paging->MapArea(cmdHeaderPhysAllocRes.value(), reinterpret_cast<uintptr_t>(cmdHeaderAllocRes.value()), 1, PTE_PRESENT | PTE_RW | PTE_PCD | PTE_NX);
		desc.CLB = reinterpret_cast<AHCI_HBACommandHeader*>(cmdHeaderAllocRes.value());

		auto fbHeaderAllocRes = virtAlloc->AllocateBlocks(1, VA_NODE_FLAG_PHYSICALLY_NOT_BACKED | VA_NODE_FLAG_MMIO | VA_NODE_FLAG_NO_EXECUTE_ACCESS | VA_NODE_FLAG_USED);
		if(!fbHeaderAllocRes)
		{
			printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to allocate virtual memory for port %d FB, error code: %lu\r\n", i, fbHeaderAllocRes.error());
			return false;
		}
		auto fbHeaderPhysAllocRes = physAlloc->AllocContiguousBlocks(1);
		if(!fbHeaderPhysAllocRes) {
			printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to allocate physical memory for port %d FB, error code: %lu\r\n", i, fbHeaderPhysAllocRes.error());
			return false;
		}
		paging->MapArea(fbHeaderPhysAllocRes.value(), reinterpret_cast<uintptr_t>(fbHeaderAllocRes.value()), 1, PTE_PRESENT | PTE_RW | PTE_PCD | PTE_NX);
		desc.FISReceiveBuffer = reinterpret_cast<uint8_t*>(fbHeaderAllocRes.value());

		desc.FISReceiveBufferSize = PAGE_SIZE;

		uintptr_t CLBPhys = paging->GetPhys(reinterpret_cast<uintptr_t>(desc.CLB)), FBPhys = paging->GetPhys(reinterpret_cast<uintptr_t>(desc.FISReceiveBuffer));

		port->PxCLB = static_cast<uint32_t>(CLBPhys);
		port->PxCLBU = static_cast<uint32_t>(CLBPhys >> 32);

		port->PxFB = static_cast<uint32_t>(FBPhys);
		port->PxFBU = static_cast<uint32_t>(FBPhys >> 32);

		// Map 32 command tables, each 4K bytes
		for(uint8_t j = 0; j < 32; j++) {
			auto cmdtAllocRes = virtAlloc->AllocateBlocks(1, VA_NODE_FLAG_PHYSICALLY_NOT_BACKED | VA_NODE_FLAG_MMIO | VA_NODE_FLAG_NO_EXECUTE_ACCESS | VA_NODE_FLAG_USED);
			if(!cmdtAllocRes)
			{
				printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to allocate virtual memory for port %d command tables, error code: %lu\r\n", i, cmdtAllocRes.error());
				return false;
			}
			auto cmdtPhysAllocRes = physAlloc->AllocContiguousBlocks(1);
			if(!cmdtPhysAllocRes) {
				printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to allocate physical memory for port %d command tables, error code: %lu\r\n", i, cmdtPhysAllocRes.error());
				return false;
			}
			paging->MapArea(cmdtPhysAllocRes.value(), reinterpret_cast<uintptr_t>(cmdtAllocRes.value()), 1, PTE_PRESENT | PTE_RW | PTE_PCD | PTE_NX);
			desc.commandTables[j] = reinterpret_cast<AHCI_CommandTable*>(cmdtAllocRes.value());
		}

		// Initialize each CLB entry

		for(uint8_t j = 0; j < 32; j++)
		{
			volatile AHCI_HBACommandHeader* header = &desc.CLB[j];
			uintptr_t commandTablesPhys = paging->GetPhys(reinterpret_cast<uintptr_t>(desc.commandTables[j]));
			header->CommandTableBase = static_cast<uint32_t>(commandTablesPhys);
			header->CommandTableBaseUpper = static_cast<uint32_t>(commandTablesPhys >> 32);
		}

		// Get port type

		desc.type = static_cast<AHCI_PortType>(port->PxSIG);

		// Enable FRE and startup port

		port->PxCMD |= AHCI_PxCMD_FISReceiveEnable;

		port->PxCMD |= AHCI_PxCMD_Start;

		int timeout = 100000;
		while (port->PxTFD & (AHCI_PxTFD_Busy | AHCI_PxTFD_DRQ) && timeout--);

		if(timeout <= 0) {
			printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to startup port %d\r\n", i);
			return false;
		}
	}

	return true;
}

bool AHCI::ReadSectors(AHCIDiskDevice* device, uint64_t lba, size_t count, void* bufferOut)
{
	if(!device || count == 0 || !bufferOut) {
		printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Invalid ReadSectors Args\r\n");
		return false;
	}

	printf("[SYSKRNL64] [SATA-AHCI] [INFO]: Trying to read from DISK at LBA 0x%llX, count %llu, buffer ptr: 0x%llX\r\n", lba, count, reinterpret_cast<uintptr_t>(bufferOut));

	const size_t maxSectorsPerCommand = 64 * KIBIBYTE / SECTOR_SIZE;
	size_t sectorsLeft = count;
	uint64_t currLba = lba;

	while(sectorsLeft > 0)
	{
		size_t sectorsThisCmd = std::min(maxSectorsPerCommand, sectorsLeft);

		AHCI_PortDesc* desc = &device->controller->ports[device->devicePort];

		volatile AHCI_Port* port = desc->port;

		int slot = -1;
		for(int i = 0; i < 32; i++)
		{
			if(!(port->PxCI & (1 << i))) {
				slot = i;
				break;
			}
		}

		if(slot < 0) {
			printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to find an available command slot for port %d for reading sectors from DISK\r\n", device->devicePort);
			return false;
		}

		volatile AHCI_HBACommandHeader* cmdHeader = &desc->CLB[slot];
		AHCI_CommandTable* cmdTable = desc->commandTables[slot];

		memset(cmdTable, 0, sizeof(AHCI_CommandTable));

		cmdHeader->CommandFISLength = sizeof(AHCI_H2DFIS) / 4;
		cmdHeader->WriteDirection = 0;
		cmdHeader->PRDTLength = 1;

		volatile AHCI_PRDTEntry* entries = cmdTable->PRDT;

		uintptr_t dataOutPhys = paging->GetPhys(reinterpret_cast<uintptr_t>(bufferOut));

		entries[0].DataBaseAddress = (uint32_t)(dataOutPhys & 0xFFFFFFFF);
		entries[0].DataBaseAddressUpper = (uint32_t)(dataOutPhys >> 32);
		entries[0].IOC_ByteCount = sectorsThisCmd * SECTOR_SIZE - 1;

		printf("dataBaseAddr: 0x%lX, upper: 0x%lX, IOC_ByteCount: 0x%lX\r\n", entries[0].DataBaseAddress, entries[0].DataBaseAddressUpper, entries[0].IOC_ByteCount);

		volatile AHCI_H2DFIS* fis = reinterpret_cast<volatile AHCI_H2DFIS*>(cmdTable->CFIS);

		fis->FISType = AHCI_FIS_TYPE_H2D;
		fis->command = AHCI_FIS_COMMAND_READ_DMA_EXT;
		fis->device = 0x40;
		fis->c = 1;
		
		fis->lba0 = currLba & 0xFF;
		fis->lba1 = (currLba >> 8) & 0xFF;
		fis->lba2 = (currLba >> 16) & 0xFF;
		fis->lba3 = (currLba >> 24) & 0xFF;
		fis->lba4 = (currLba >> 32) & 0xFF;
		fis->lba5 = (currLba >> 40) & 0xFF;

		fis->count = sectorsThisCmd;

		port->PxCI |= (1 << slot);

		uint32_t timeout = 1000000;
		while(port->PxCI & (1 << slot) && timeout--);

		if(timeout <= 0) {
			printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to read from DISK at port %d: timeout\r\n", device->devicePort);
			return false;
		}

		if(port->PxIS & AHCI_PxIS_TaskFileErrorStatus) {
			printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to read from DISK at port %d: Task File Error\r\n", device->devicePort);
			return false;
		}
		
		if(port->PxSERR != 0) {
			printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to read from DISK at port %d, PxSERR: 0x%lX\r\n", device->devicePort, port->PxSERR);
			return false;
		}

		if((port->PxTFD & 0xFF) & AHCI_PxTFD_Error) {
			printf("[SYSKRNL64] [SATA-AHCI] [ERROR]: Failed to read from DISK at port %d, Status: 0x%lX, Error: 0x%lX\r\n", device->devicePort, (port->PxTFD & 0xFF), ((port->PxTFD >> 8) & 0xFF));
			return false;
		}

		bufferOut = static_cast<uint8_t*>(bufferOut) + sectorsThisCmd * SECTOR_SIZE;
		currLba += sectorsThisCmd;
		sectorsLeft -= sectorsThisCmd;
	}

	return true;
}