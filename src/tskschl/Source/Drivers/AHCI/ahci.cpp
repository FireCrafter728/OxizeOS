#include <Drivers/AHCI/ahci.hpp>

using namespace TskSchl::AHCI;

constexpr size_t hbaBlocks = BLOCK_COUNT(sizeof(HBA));

AHCI::AHCI(AHCIDevice* device)
{
    if(!Initialize(device)) HaltSystem();
}

bool AHCI::Initialize(AHCIDevice* device)
{
    if(!device || !device->deviceInfo) {
        printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Invalid initializer args\r\n");
        return false;
    }

    // Retrieve PCIe BAR 5 and confirm it's integrity
    
    PCIe::BARInfo bar = device->deviceInfo->BARs[5];

    if(bar.BarType != PCIe::BARTypes::MMIO) {
        printf("[TSKSCHL] [SATA-AHCI] [ERROR]: AHCI BAR 5 is an I/O BAR, not an MMIO BAR\r\n");
        return false;
    }

    // Map PCIe BAR 5 to virtual memory

    device->hba = reinterpret_cast<HBA*>(mmd->malloc(hbaBlocks, MMD::MT_MMIO));

    if(!device->hba) {
        printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to reserve virtual memory for HBA\r\n");
        return false;
    }

    paging->MapArea(bar.BarAddr, reinterpret_cast<uintptr_t>(device->hba), hbaBlocks, PTE_PRESENT | PTE_RW | PTE_CD | PTE_NX);

    //
    // Initializing AHCI
    //

    // Step 1: Enable Memory Space & Bus Master bits in PCIe Config Space command register
    device->deviceInfo->root->Command |= PCIe::ConfigCommandMMIOEnable | PCIe::ConfigCommandBusMasterEnable;

    // Step 2: Reset all ports to clean up after UEFI

    if(!ResetPorts(device)) {
        printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to reset ports\r\n");
        return false;
    }

    // Step 3: Reset the entire HBA

    device->hba->GlobalControl |= GHC_HBAReset;

    int timeout = 100000;
    while(timeout-- && (device->hba->GlobalControl & GHC_HBAReset) != 0);
    if(timeout <= 0) {
        printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to reset AHCI GHC\r\n");
        return false;
    }

    // Step 4: Enable AHCI In HBA

    device->hba->GlobalControl |= GHC_AHCIEnable;
    if((device->hba->GlobalControl &GHC_AHCIEnable) == 0) {
        printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to enable AHCI mode\r\n");
        return false;
    }

    // Step 5: Initialize all ports

    if(!InitializePorts(device)) {
        printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to initialize ports\r\n");
        return false;
    }

    // Step 6: Send IDENTIFY DEVICE for each port

    for(uint8_t i = 0; i < 32; i++)
    {
        if(!(device->hba->PortsImplemented & (1U << i))) continue;

        PortDesc* desc = &device->ports[i];

        volatile Port* port = desc->port;

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
            printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to find available slots for IDENTIFY DEVICE command for port %d\r\n", i);
            return false;
        }

        // Make sure port is ready for a command

        while(port->PxTFD & PxTFD_Busy);
        while(port->PxTFD & PxTFD_DRQ);

        // Get command header & table

        volatile HBACommandHeader* commandHeader = &desc->CLB[slot];
        uintptr_t cmdTablePhys = ((uintptr_t)commandHeader->CommandTableBase | ((uintptr_t)commandHeader->CommandTableBaseUpper << 32));
        CommandTable* cmdTable = reinterpret_cast<CommandTable*>(paging->GetVirt(cmdTablePhys));

        // Setup command header

        commandHeader->PRDTLength = 1;
        commandHeader->WriteDirection = 0;
        commandHeader->CommandFISLength = sizeof(H2DFIS) / 4;

        // Setup command table

        PRDTEntry* entry0 = &cmdTable->PRDT[0];

        uintptr_t dataOutPhys = paging->GetPhys(reinterpret_cast<uintptr_t>(desc->IdentifyBuffer));

        entry0->DataBaseAddress = (uint32_t)(dataOutPhys & 0xFFFFFFFF);
        entry0->DataBaseAddressUpper = (uint32_t)(dataOutPhys >> 32);
        entry0->ByteCount = 511;

        volatile H2DFIS* cfis = reinterpret_cast<H2DFIS*>(cmdTable->CFIS);

        memset((void*)cmdTable->CFIS, 0, 64);

        cfis->FISType = FIS_TYPE_H2D;
        cfis->c = 1;
        cfis->command = FIS_COMMAND_IDENTIFY;
        cfis->device = 0;

        port->PxIS = 0xFFFFFFFF;
        port->PxCI |= (1U << slot);

        // Command sent, wait for response

        int timeout = 1000000;
        while((port->PxCI & (1U << slot)) && timeout--);
        if(timeout <= 0)
        {
            printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to send IDENTIFY DEVICE for port %d, timeout\r\n", i);
            return false;
        }

        if(port->PxSERR != 0) {
            printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to send IDENTIFY DEVICE for port %d, PxSERR: 0x%X\r\n", i, port->PxSERR);
            return false;
        }

        if(port->PxIS & PxIS_TaskFileErrorStatus) {
            printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to send IDENTIFY DEVICE for port %d, Task File Error\r\n", i);
            return false;
        }

        if((port->PxTFD & 0xFF) & PxTFD_Error) {
            printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to send IDENTIFY DEVICE for port %d, Status: 0x%X, Error: 0x%X\r\n", i, (port->PxTFD & 0xFF), ((port->PxTFD >> 8) & 0xFF));
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

        volatile Port* port = &device->hba->Ports[i];

        port->PxCMD &= ~(PxCMD_Start | PxCMD_FISReceiveEnable);

        while(port->PxCMD & (PxCMD_CommandListRunning | PxCMD_FISReceiveRunning));

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
            printf("[TSKSCHL] [SATA-AHCI] [WARN]: Port %d is implemented but unresponsive\r\n", i);
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

        PortDesc& desc = device->ports[i];

        desc.port = &device->hba->Ports[i];

        volatile Port* port = desc.port;

        if((port->PxSSTS & 0x0F) != 3 || ((port->PxSSTS >> 8) & 0x0F) != 1) continue; // Port not present or active, go to the next one

        // Setup CLB & FB

        desc.CLB = reinterpret_cast<HBACommandHeader*>(mmd->malloc(1, MMD::MT_KRNL, PTE_PRESENT | PTE_RW | PTE_CD | PTE_NX));
        if(!desc.CLB) {
            printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to allocate memory for port %d CLB\r\n", i);
            return false;
        }

        desc.FISReceiveBuffer = reinterpret_cast<uint8_t*>(mmd->malloc(1, MMD::MT_KRNL, PTE_PRESENT | PTE_RW | PTE_CD | PTE_NX));
        if(!desc.FISReceiveBuffer) {
            printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to allocate memory for port %d FB\r\n", i);
            return false;
        }

        desc.FISReceiveBufferSize = PAGE_SIZE;

        uintptr_t CLBPhys = paging->GetPhys(reinterpret_cast<uintptr_t>(desc.CLB)), FBPhys = paging->GetPhys(reinterpret_cast<uintptr_t>(desc.FISReceiveBuffer));

        port->PxCLB = static_cast<uint32_t>(CLBPhys);
        port->PxCLBU = static_cast<uint32_t>(CLBPhys >> 32);

        port->PxFB = static_cast<uint32_t>(FBPhys);
        port->PxFBU = static_cast<uint32_t>(FBPhys >> 32);

        // Map 32 command tables, each 4K bytes
        for(uint8_t j = 0; j < 32; j++) {
            desc.commandTables[j] = reinterpret_cast<CommandTable*>(mmd->malloc(1, MMD::MT_KRNL, PTE_PRESENT | PTE_RW | PTE_CD | PTE_NX));

            if(!desc.commandTables[j]) {
                printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to allocate memory for port %d command tables %d\r\n", i, j);
                return false;
            }
        }

        // Initialize each CLB entry

        for(uint8_t j = 0; j < 32; j++)
        {
            volatile HBACommandHeader* header = &desc.CLB[j];
            uintptr_t commandTablesPhys = paging->GetPhys(reinterpret_cast<uintptr_t>(desc.commandTables[j]));
            header->CommandTableBase = static_cast<uint32_t>(commandTablesPhys);
            header->CommandTableBaseUpper = static_cast<uint32_t>(commandTablesPhys >> 32);
        }

        // Get port type

        desc.type = static_cast<PortType>(port->PxSIG);

        // Enable FRE and startup port

        port->PxCMD |= PxCMD_FISReceiveEnable;

        port->PxCMD |= PxCMD_Start;

        int timeout = 100000;
        while (port->PxTFD & (PxTFD_Busy | PxTFD_DRQ) && timeout--);

        if(timeout <= 0) {
            printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to startup port %d\r\n", i);
            return false;
        }
    }

    return true;
}

bool AHCI::ReadSectors(AHCIDiskDevice* device, uint64_t lba, size_t count, void* bufferOut)
{
    if(!device || count == 0 || !bufferOut) {
        printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Invalid ReadSectors Args\r\n");
        return false;
    }

    const size_t maxSectorsPerCommand = 64 * KIBIBYTE / SECTOR_SIZE;
    size_t sectorsLeft = count;
    uint64_t currLba = lba;

    while(sectorsLeft > 0)
    {
        size_t sectorsThisCmd = std::min(maxSectorsPerCommand, sectorsLeft);

        PortDesc* desc = &device->controller->ports[device->devicePort];

        volatile Port* port = desc->port;

        int slot = -1;
        for(int i = 0; i < 32; i++)
        {
            if(!(port->PxCI & (1 << i))) {
                slot = i;
                break;
            }
        }

        if(slot < 0) {
            printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to find an available command slot for port %d for reading sectors from DISK\r\n", device->devicePort);
            return false;
        }

        volatile HBACommandHeader* cmdHeader = &desc->CLB[slot];
        uintptr_t cmdTablePhys = ((uintptr_t)cmdHeader->CommandTableBase | ((uintptr_t)cmdHeader->CommandTableBaseUpper << 32));
        CommandTable* cmdTable = reinterpret_cast<CommandTable*>(paging->GetVirt(cmdTablePhys));

        memset(cmdTable, 0, sizeof(CommandTable));

        cmdHeader->CommandFISLength = sizeof(H2DFIS) / 4;
        cmdHeader->WriteDirection = 0;
        cmdHeader->PRDTLength = 1;

        volatile PRDTEntry* entries = cmdTable->PRDT;

        uintptr_t dataOutPhys = paging->GetPhys(reinterpret_cast<uintptr_t>(bufferOut));

        entries[0].DataBaseAddress = (uint32_t)(dataOutPhys & 0xFFFFFFFF);
        entries[0].DataBaseAddressUpper = (uint32_t)(dataOutPhys >> 32);
        entries[0].ByteCount = std::min(sectorsThisCmd * SECTOR_SIZE, 64ULL * KIBIBYTE);

        H2DFIS* fis = reinterpret_cast<H2DFIS*>(cmdTable->CFIS);

        fis->FISType = FIS_TYPE_H2D;
        fis->command = FIS_COMMAND_READ_DMA_EXT;
        fis->device = 0x40;

        
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
            printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to read from DISK at port %d: timeout\r\n", device->devicePort);
            return false;
        }

        if(port->PxIS & PxIS_TaskFileErrorStatus) {
            printf("[TSKSCHL] [SATA-AHCI] [ERROR]: Failed to read from DISK at port %d: Task File Error\r\n", device->devicePort);
        }

        bufferOut = static_cast<uint8_t*>(bufferOut) + sectorsThisCmd * SECTOR_SIZE;
        currLba += sectorsThisCmd;
        sectorsLeft -= sectorsThisCmd;
    }

    return true;
}