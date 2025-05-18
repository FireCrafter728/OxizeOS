#include <DISK/ATA_PIO.h>
#include <io.h>
#include <stdio.h>

typedef enum {
    ATA_PIO_DATA_REG                            = 0x1F0,
    ATA_PIO_SECTOR_COUNT_REG                    = 0x1F2,
    ATA_PIO_LBA_LOW_REG                         = 0x1F3,
    ATA_PIO_LBA_MID_REG                         = 0x1F4,
    ATA_PIO_LBA_HIGH_REG                        = 0x1F5,
    ATA_PIO_DRIVE_HEAD_REG                      = 0x1F6,
    
    ATA_PIO_READ_ERR_REG                        = 0x1F1,
    ATA_PIO_WRITE_FEATURE_REG                   = 0x1F1,
    
    ATA_PIO_READ_STATUS_REG                     = 0x1f7,
    ATA_PIO_WRITE_COMMAND_REG                   = 0x1f7,
    
    ATA_PIO_READ_ALTERNATE_STATUS_REG           = 0x3F6,
    ATA_PIO_WRITE_DEVICE_CONTROL_REG            = 0x3F6,
} ATA_PIO_Bus;

#define ATA_PIO_MASTER                          0

typedef enum {
    ATA_PIO_COMMAND_IDENTIFY                    = 0xEC,
    ATA_PIO_COMMAND_READ_SECTORS                = 0x20,
} ATA_PIO_Command;

#define ATA_PIO_DEVICE_READY                    0
#define ATA_PIO_DEVICE_NOT_READY                1

uint8_t ATA_PIO_WaitCommandEnd()
{
    uint8_t status;
    do {
        status = inb(ATA_PIO_READ_STATUS_REG);
    } while (status & 0x80);
    if(status & 0x08) return ATA_PIO_DEVICE_READY;
    return ATA_PIO_DEVICE_NOT_READY;
}

void ATA_PIO_Wait400ns()
{
    for(int i = 0; i < 4; i++) inb(ATA_PIO_READ_ALTERNATE_STATUS_REG);
}

bool ATA_PIO_Initialize(ATA_PIO_Device* device)
{
    outb(ATA_PIO_DRIVE_HEAD_REG, 0xA0 | (ATA_PIO_MASTER << 4));
    ATA_PIO_Wait400ns();

    outb(ATA_PIO_SECTOR_COUNT_REG, 0);
    outb(ATA_PIO_LBA_LOW_REG, 0);
    outb(ATA_PIO_LBA_MID_REG, 0);
    outb(ATA_PIO_LBA_HIGH_REG, 0);

    outb(ATA_PIO_READ_STATUS_REG, ATA_PIO_COMMAND_IDENTIFY);
    if(ATA_PIO_WaitCommandEnd() == ATA_PIO_DEVICE_NOT_READY) {
        printf("[ATA PIO] [ERROR]: ATA PIO Device is not ready\r\n");
        return false;
    }

    for(int i = 0; i < SECTOR_SIZE_WORD; i++) device->IdentifyData[i] = inw(ATA_PIO_DATA_REG);

    if((device->IdentifyData[0] & 0x8000) != 0) {
        printf("[ATA PIO] [ERROR]: ATAPI Device Not supported, please use an ATA device\r\n");
        return false;
    }

    if((device->IdentifyData[49] & (1 << 9)) == 0) {
        printf("[ATA PIO] [ERROR]: LBA28 Is not supported on the ATA Device");
        return false;
    }

    for(int i = 0; i < 40; i += 2) {
        device->driveInfo.model[i] = (device->IdentifyData[i / 2 + 27] >> 8) & 0xFF;
        device->driveInfo.model[i + 1] = device->IdentifyData[27 + i / 2] & 0xFF;
    }
    device->driveInfo.model[40] = '\0';

    device->driveInfo.sectorCount = device->IdentifyData[60] | ((uint32_t)device->IdentifyData[61] << 16);
    device->Initialized = true;
    return true;
}

bool ATA_PIO_ReadSectors(ATA_PIO_Device* device, uint32_t lba, uint8_t count, void* dataOut)
{
    if(!device->Initialized) {
        printf("[ATA PIO] [ERROR]: Device must be initialized before use\r\n");
        return false;
    }

    outb(ATA_PIO_DRIVE_HEAD_REG, 0xE0 | ((lba >> 24) & 0x0F));
    ATA_PIO_Wait400ns();
    outb(ATA_PIO_SECTOR_COUNT_REG, count);
    outb(ATA_PIO_LBA_LOW_REG, (uint8_t)(lba & 0xFF));
    outb(ATA_PIO_LBA_MID_REG, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PIO_LBA_HIGH_REG, (uint8_t)((lba >> 16) & 0xFF));

    outb(ATA_PIO_WRITE_COMMAND_REG, ATA_PIO_COMMAND_READ_SECTORS);

    if(ATA_PIO_WaitCommandEnd() == ATA_PIO_DEVICE_NOT_READY) {
        printf("[ATA PIO] [ERROR]: ATA PIO Device is not ready\r\n");
        return false;
    }

    uint16_t* dataBuffer = (uint16_t*)dataOut;
    for(uint32_t i = 0; i < (count * SECTOR_SIZE_WORD); i++) {
        dataBuffer[i] = inw(ATA_PIO_DATA_REG);
    }

    return true;
}