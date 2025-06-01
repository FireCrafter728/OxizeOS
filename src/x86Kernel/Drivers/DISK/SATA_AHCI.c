#include <DISK/SATA_AHCI.h>
#include <stdio.h>
#include <io.h>
#include <Memory/memdefs.h>
#include <memory.h>

#define SATA_AHCI_GHC_OFFSET 0x04

#define SATA_AHCI_GHC_ResetController (1 << 0)
#define SATA_AHCI_GHC_EnableAHCI (1 << 31)

#define SATA_AHCI_PI_OFFSET 0x0C
#define SATA_AHCI_PORT_REG_SIZE 0x80
#define SATA_AHCI_PORT_BASE_OFFSET 0x100

#define SATA_AHCI_MAX_PORTS 32

#define SATA_AHCI_PxCMD 0x18
#define SATA_AHCI_PxSERR 0x30
#define SATA_AHCI_PxIS 0x10
#define SATA_AHCI_PxCLB 0x00
#define SATA_AHCI_PxFB 0x08
#define SATA_AHCI_PxSTS 0x28
#define SATA_AHCI_PxSIG 0x24
#define SATA_AHCI_PxCI 0x38
#define SATA_AHCI_PxTFD 0x20

#define SATA_DEV_PRESENT 0x3
#define SATA_IPM_ACTIVE 0x1

#define SATA_AHCI_PORT_SIG_SATA 0x00000101
#define SATA_AHCI_PORT_SIG_ATAPI 0xEB140101

#define SATA_AHCI_CMD_READ_DMA_EXT 0x25

typedef enum
{
    SATA_AHCI_CMD_IDENTIFY = 0x00,
    SATA_AHCI_CMD_READ = 0x01,
} SATA_AHCI_PORT0_COMMANDS;

void SATA_EnableAHCI(SATA_AHCI_Device *device)
{
    uint16_t command = PCI_ReadConfigWord(&device->info, 0x04);

    command |= (1 << 1) | (1 << 2);

    PCI_WriteConfigWord(&device->info, 0x04, command);
}

void SATA_MapAHCI_BAR5(SATA_AHCI_Device *device)
{
    device->AHCI_BAR = (volatile uint32_t *)PCI_ReadBAR(&device->info, PCI_BAR5_OFFSET);
    printf("[SATA AHCI] [INFO]: AHCI MMIO Base address: 0x%lx\r\n", (unsigned long)device->AHCI_BAR);
}

bool SATA_AHCI_ResetController(SATA_AHCI_Device *device)
{
    volatile uint32_t *ghc = &device->AHCI_BAR[SATA_AHCI_GHC_OFFSET / 4];

    *ghc |= SATA_AHCI_GHC_ResetController;

    int timeout = 1000000;
    while ((*ghc & SATA_AHCI_GHC_ResetController) != 0 && timeout--)
        iowait(); // Use the IOWAIT function that writes some data into port 0x80
    if (timeout <= 0)
    {
        printf("[SATA AHCI] [ERROR]: Failed to reset controller, timeout\r\n");
        return false;
    }
    return true;
}

bool SATA_EnableAHCIMode(SATA_AHCI_Device *device)
{
    volatile uint32_t *ghc = &device->AHCI_BAR[SATA_AHCI_GHC_OFFSET / 4];

    *ghc |= SATA_AHCI_GHC_EnableAHCI;

    if ((*ghc & SATA_AHCI_GHC_EnableAHCI) == 0)
    {
        printf("[SATA AHCI] [ERROR]: Failed to enable AHCI mode\r\n");
        return false;
    }

    return true;
}

void SATA_AHCI_InitializePorts(SATA_AHCI_Device *device)
{
    volatile uint32_t *pi_reg = &device->AHCI_BAR[SATA_AHCI_PI_OFFSET / 4];
    uint32_t portsImplemented = *pi_reg;
    uint8_t Ports[SATA_AHCI_USED_PORT_COUNT];
    uint8_t num = 0;
    for (int i = 0; i < SATA_AHCI_MAX_PORTS; i++)
        if (portsImplemented & (1 << i))
            Ports[num++] = i;

    uint32_t ImplementedPorts = num;

    printf("[SATA AHCI] [INFO]: AHCI Ports implemented: %lu\r\n", ImplementedPorts);

    uint32_t CommandListsAddr = SATA_AHCI_COMMAND_LISTS_ADDR;
    uint32_t FisAddr = SATA_AHCI_FIS_ADDR;

    memset((void *)SATA_AHCI_COMMAND_LISTS_ADDR, 0, SATA_AHCI_COMMAND_LISTS_SIZE);
    memset((void *)SATA_AHCI_FIS_ADDR, 0, SATA_AHCI_FIS_SIZE);
    memset((void *)SATA_AHCI_COMMAND_TABLES_ADDR, 0, SATA_AHCI_COMMAND_TABLES_SIZE);

    for (uint8_t i = 0; i < ImplementedPorts && i < SATA_AHCI_USED_PORT_COUNT; i++)
    {
        uint8_t port = Ports[i];
        device->AHCIPorts[port].port_number = port;
        device->AHCIPorts[port].port_mmio = &device->AHCI_BAR[(SATA_AHCI_PORT_BASE_OFFSET / 4) + (port * (SATA_AHCI_PORT_REG_SIZE / 4))];

        device->AHCIPorts[port].port_mmio[SATA_AHCI_PxCMD / 4] &= ~((1 << 0) | (1 << 4));

        while ((device->AHCIPorts[port].port_mmio[SATA_AHCI_PxCMD / 4] & ((1 << 15) | (1 << 14))) != 0)
            ;

        device->AHCIPorts[port].port_mmio[SATA_AHCI_PxSERR / 4] = 0xFFFFFFFF;
        device->AHCIPorts[port].port_mmio[SATA_AHCI_PxIS / 4] = 0xFFFFFFFF;

        device->AHCIPorts[port].port_mmio[SATA_AHCI_PxCLB / 4] = CommandListsAddr;

        device->AHCIPorts[port].commandHeaders = (SATA_AHCI_CommandHeader *)(uintptr_t)(CommandListsAddr);
        CommandListsAddr += SATA_AHCI_COMMAND_LISTS_UNIT_SIZE;

        device->AHCIPorts[port].port_mmio[SATA_AHCI_PxFB / 4] = FisAddr;

        FisAddr += SATA_AHCI_FIS_UNIT_SIZE;

        uint32_t TableAddr = SATA_AHCI_COMMAND_TABLES_ADDR + SATA_AHCI_COMMAND_TABLES_PORT_SIZE * port;
        for (uint8_t slot = 0; slot < SATA_AHCI_COMMAND_SLOTS; slot++)
        {
            device->AHCIPorts[port].commandTables[slot] = (SATA_AHCI_CommandTable *)TableAddr;

            SATA_AHCI_CommandHeader *header = &device->AHCIPorts[port].commandHeaders[slot];
            header->CommandTableBaseAddressLow = TableAddr;
            header->CommandTableBaseAddressHigh = 0;

            TableAddr += SATA_AHCI_COMMAND_TABLES_UNIT_SIZE;
        }

        device->AHCIPorts[port].port_mmio[SATA_AHCI_PxCMD / 4] |= (1 << 4) | (1 << 0);

        uint32_t portStatus = device->AHCIPorts[port].port_mmio[SATA_AHCI_PxSTS / 4];
        uint32_t portSignature = device->AHCIPorts[port].port_mmio[SATA_AHCI_PxSIG / 4];

        uint8_t det = portStatus & 0x0F;
        uint8_t ipm = (portStatus >> 8) & 0x0F;

        device->AHCIPorts[port].deviceActive = false;

        if (det != SATA_DEV_PRESENT || ipm != SATA_IPM_ACTIVE)
            printf("[SATA AHCI] [WARN]: Port %u - No active device\r\n", port);
        else
            device->AHCIPorts[port].deviceActive = true;

        if (portSignature != SATA_AHCI_PORT_SIG_SATA && portSignature != SATA_AHCI_PORT_SIG_ATAPI)
            printf("[SATA AHCI] [WARN]: Port %d: Unknown signature %lu\r\n", port, portSignature);
    }
}

bool SATA_AHCI_Identify(SATA_AHCI_Device *device, void *dataOut)
{
    SATA_AHCI_Port *port = &device->AHCIPorts[0];

    port->port_mmio[SATA_AHCI_PxIS / 4] = 0xFFFFFFFF;
    SATA_AHCI_CommandHeader *cmdHeader = &port->commandHeaders[SATA_AHCI_CMD_IDENTIFY];
    cmdHeader->CommandFISLength = 5;
    cmdHeader->WriteDirection = 0;
    cmdHeader->PRDTLength = 1;

    SATA_AHCI_CommandTable *cmdTable = port->commandTables[SATA_AHCI_CMD_IDENTIFY];

    cmdTable->PRDTEntries[0].DataBaseAddressLow = (uint32_t)(uintptr_t)dataOut;
    cmdTable->PRDTEntries[0].DataBaseAddressHigh = 0;
    cmdTable->PRDTEntries[0].ByteCountAndInterrupt = (SECTOR_SIZE - 1) | (1u << 31);

    SATA_AHCI_COMMAND_FIS *fis = (SATA_AHCI_COMMAND_FIS *)&cmdTable->CommandFIS[0];
    fis->fisType = 0x27;
    fis->commandControlBit = 1;
    fis->ataCommandCode = 0xEC;
    fis->deviceRegister = 0x00;

    port->port_mmio[SATA_AHCI_PxCI / 4] = (1u << SATA_AHCI_CMD_IDENTIFY);

    int timeout = 1000000;
    while ((port->port_mmio[SATA_AHCI_PxCI / 4] & (1u << SATA_AHCI_CMD_IDENTIFY)) && timeout)
        timeout--;
    if (timeout <= 0)
    {
        printf("[SATA AHCI] [ERROR]: Failed to send IDENTIFY: Timeout\r\n");
        return false;
    }

    if (port->port_mmio[SATA_AHCI_PxSERR] != 0)
    {
        printf("[SATA AHCI] [ERROR]: Failed to send IDENTIFY, error code: 0x%lx\r\n", port->port_mmio[SATA_AHCI_PxSERR]);
        return false;
    }
    if (port->port_mmio[SATA_AHCI_PxIS / 4] & (1u << 30))
    {
        printf("[SATA AHCI] [ERROR]: Failed to send IDENTIFY, Task File Error\r\n");
        return false;
    }

    if ((port->port_mmio[SATA_AHCI_PxTFD / 4] & 0xFF) & (1 << 0))
    {
        printf("[SATA AHCI] [ERROR]: Failed to send IDENTIFY, Status: 0x%lx, Error: 0x%lx", (port->port_mmio[SATA_AHCI_PxTFD / 4] & 0xFF), ((port->port_mmio[SATA_AHCI_PxTFD] >> 8) & 0xFF));
        return false;
    }

    return true;
}

bool SATA_AHCI_Initialize(SATA_AHCI_Device *device)
{

    if (device->info.classCode != SATA_AHCI_MASS_STORAGE_CONTROLLER || device->info.subclass != SATA_AHCI_SATA_CONTROLLER || device->info.progIF != SATA_AHCI_1_0_CONTROLLER)
    {
        bool found = false;
        for (uint16_t i = 0; i < PCI_MAX_BUSSES && !found; i++)
            for (uint8_t j = 0; j < PCI_MAX_DEVICES && !found; j++)
                for (uint8_t k = 0; k < PCI_MAX_FUNCTIONS && !found; k++)
                {
                    if (!PCI_GetDeviceInfo(&device->info, i, j, k))
                        continue;
                    if (device->info.classCode == SATA_AHCI_MASS_STORAGE_CONTROLLER && device->info.subclass == SATA_AHCI_SATA_CONTROLLER && device->info.progIF == SATA_AHCI_1_0_CONTROLLER)
                    {
                        found = true;
                        printf("[SATA AHCI] [INFO]: Found SATA AHCI Device in PCI\r\n");
                        break;
                    }
                }
        if (!found) return false;
    }

    SATA_EnableAHCI(device);
    SATA_MapAHCI_BAR5(device);
    if (!SATA_AHCI_ResetController(device))
        return false;
    if (!SATA_EnableAHCIMode(device))
        return false;
    SATA_AHCI_InitializePorts(device);

    if (!SATA_AHCI_Identify(device, &device->IdentifyData))
        return false;

    device->driveInfo.sectorCount = device->IdentifyData[120] | (device->IdentifyData[121] << 8) | (device->IdentifyData[122] << 16) | (device->IdentifyData[123] << 24);

    for (int i = 0; i < 20; i++)
    {
        int baseIndex = 54 + i * 2;
        device->driveInfo.model[i * 2] = device->IdentifyData[baseIndex + 1];
        device->driveInfo.model[i * 2 + 1] = device->IdentifyData[baseIndex];
    }

    device->driveInfo.model[40] = '\0';

    return true;
}

bool SATA_AHCI_ReadSectors(SATA_AHCI_Device *device, uint32_t lba, uint16_t count, void *dataOut)
{
    SATA_AHCI_Port *port = &device->AHCIPorts[0];
    port->port_mmio[SATA_AHCI_PxIS / 4] = 0xFFFFFFFF;

    SATA_AHCI_CommandHeader *cmdHeader = &port->commandHeaders[SATA_AHCI_CMD_READ];
    cmdHeader->CommandFISLength = 5;
    cmdHeader->WriteDirection = 0;
    cmdHeader->PRDTLength = 1;

    SATA_AHCI_CommandTable *cmdTable = port->commandTables[SATA_AHCI_CMD_READ];
    cmdTable->PRDTEntries[0].DataBaseAddressLow = (uint32_t)(uintptr_t)dataOut;
    cmdTable->PRDTEntries[0].DataBaseAddressHigh = 0;
    cmdTable->PRDTEntries[0].ByteCountAndInterrupt = ((count * SECTOR_SIZE) - 1) | (1u << 31);

    SATA_AHCI_COMMAND_FIS *fis = (SATA_AHCI_COMMAND_FIS *)&cmdTable->CommandFIS[0];
    fis->fisType = 0x27;
    fis->commandControlBit = 1;
    fis->ataCommandCode = SATA_AHCI_CMD_READ_DMA_EXT;

    fis->LBALow0 = (uint8_t)(lba & 0xFF);
    fis->LBALow1 = (uint8_t)((lba >> 8) & 0xFF);
    fis->LBALow2 = (uint8_t)((lba >> 16) & 0xFF);
    fis->deviceRegister = 1 << 6;

    fis->LBAHigh3 = (uint8_t)((lba >> 24) & 0xFF);
    fis->LBAHigh4 = 0;
    fis->LBAHigh5 = 0;

    fis->sectorCountLow = (uint8_t)(count & 0xFF);
    fis->sectorCountHigh = (uint8_t)((count >> 8) & 0xFF);

    port->port_mmio[SATA_AHCI_PxCI / 4] = (1u << SATA_AHCI_CMD_READ);

    int timeout = 1000000;
    while ((port->port_mmio[SATA_AHCI_PxCI / 4] & (1u << SATA_AHCI_CMD_READ)) && timeout)
        timeout--;

    if (timeout <= 0)
    {
        printf("[SATA AHCI] [ERROR]: Failed to read from DISK, Timeout\r\n");
        return false;
    }

    if (port->port_mmio[SATA_AHCI_PxSERR] != 0)
    {
        printf("[SATA AHCI] [ERROR]: Failed to read from DISK, error code: 0x%lx\r\n", port->port_mmio[SATA_AHCI_PxSERR]);
        return false;
    }

    if (port->port_mmio[SATA_AHCI_PxIS / 4] & (1u << 30))
    {
        printf("[SATA AHCI] [ERROR]: Failed to read from DISK, Task File Error\r\n");
        return false;
    }

    if ((port->port_mmio[SATA_AHCI_PxTFD / 4] & 0xFF) & (1 << 0))
    {
        printf("[SATA AHCI] [ERROR]: Failed to read from DISK, Read sectors status Error\r\n");
        return false;
    }

    return true;
}