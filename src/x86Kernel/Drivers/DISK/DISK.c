#include <DISK/DISK.h>
#include <PCI/pci.h>
#include <stdio.h>

bool DISK_Initialize(DISK *disk)
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
                    found = true;
                    printf("[DISK] [INFO]: SATA AHCI Controller found\r\n");
                    break;
                }
            }

    if (!found)
    {
        printf("[DISK] [ERROR]: Failed to find a supported SATA AHCI device\r\n");
        return false;
    }

    disk->device.info = info;
    if (!SATA_AHCI_Initialize(&disk->device))
        return false;

    return true;
}

bool DISK_ReadSectors(DISK *disk, uint32_t lba, uint16_t count, void *dataOut)
{
    if (!SATA_AHCI_ReadSectors(&disk->device, lba, count, dataOut))
        return false;

    return true;
}