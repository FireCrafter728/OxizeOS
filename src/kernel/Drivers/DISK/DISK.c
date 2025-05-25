#include <DISK/DISK.h>
#include <PCI/pci.h>
#include <stdio.h>

bool DISK_Initialize(DISK* disk)
{
    PCI_DeviceInfo info;
    bool found = false;

    for (uint16_t i = 0; i < PCI_MAX_BUSSES && !found; i++)
        for (uint8_t j = 0; j < PCI_MAX_DEVICES && !found; j++)
            for (uint8_t k = 0; k < PCI_MAX_FUNCTIONS && !found; k++)
            {
                if (!PCI_GetDeviceInfo(&info, i, j, k))
                    continue;
                if (info.classCode == SATA_AHCI_MASS_STORAGE_CONTROLLER && info.subclass == SATA_AHCI_SATA_CONTROLLER && info.progIF == SATA_AHCI_1_0_CONTROLLER)
                {
                    disk->DeviceType = DISK_CONTROLLER_SATA_AHCI;
                    found = true;
                    printf("[DISK] [INFO]: SATA AHCI Controller found\r\n");
                    break;
                }
                else if(info.classCode == SATA_AHCI_MASS_STORAGE_CONTROLLER && info.subclass == 0x06 && info.progIF == 0x00)
                {
                    disk->DeviceType = DISK_CONTROLLER_SATA_IDE;
                    found = true;
                    printf("[DISK] [INFO]: SATA IDE Controller found\r\n");
                    break;
                }
                else if(info.classCode == SATA_AHCI_MASS_STORAGE_CONTROLLER && info.subclass == 0x01)
                {
                    disk->DeviceType = DISK_CONTROLLER_IDE;
                    found = true;
                    printf("[DISK] [INFO]: IDE Controller found\r\n");
                    break;
                }
            }

    if(!found) {
        printf("[DISK] [ERROR]: Failed to find a supported device\r\n");
        return false;
    }

    switch(disk->DeviceType)
    {
        case DISK_CONTROLLER_SATA_AHCI:
        {
            disk->SATAAhci.info = info;
            if(!SATA_AHCI_Initialize(&disk->SATAAhci)) return false;
            break;
        }
        case DISK_CONTROLLER_SATA_IDE:
        case DISK_CONTROLLER_IDE:
        {
            if(!ATA_PIO_Initialize(&disk->ATAPio)) return false;
            break;
        }
        default: return false;
    }

    return true;
}

bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint8_t count, void* dataOut)
{
    switch(disk->DeviceType)
    {
        case DISK_CONTROLLER_SATA_AHCI:
        {
            if(!SATA_AHCI_ReadSectors(&disk->SATAAhci, lba, count, dataOut)) return false;
            break;
        }
        case DISK_CONTROLLER_SATA_IDE:
        case DISK_CONTROLLER_IDE:
        {
            if(!ATA_PIO_ReadSectors(&disk->ATAPio, lba, count, dataOut)) return false;
            break;
        }
        default: return false;
    }

    return true;
}