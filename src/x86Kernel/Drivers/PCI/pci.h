#pragma once
#include <stdint.h>
#include <stdbool.h>

#define PCI_MAX_BUSSES      256
#define PCI_MAX_DEVICES     32
#define PCI_MAX_FUNCTIONS   8

#define PCI_BAR0_OFFSET     0x10
#define PCI_BAR1_OFFSET     0x14
#define PCI_BAR2_OFFSET     0x18
#define PCI_BAR3_OFFSET     0x1C
#define PCI_BAR4_OFFSET     0x20
#define PCI_BAR5_OFFSET     0x24

typedef struct
{
    uint16_t vendorID;
    uint16_t deviceID;
    uint8_t classCode;
    uint8_t subclass;
    uint8_t progIF;
    uint8_t bus, device, function;
} PCI_DeviceInfo;

bool PCI_GetDeviceInfo(PCI_DeviceInfo* info, uint8_t bus, uint8_t device, uint8_t function);

uint16_t PCI_ReadConfigWord(PCI_DeviceInfo* info, uint8_t offset);
void PCI_WriteConfigWord(PCI_DeviceInfo* info, uint8_t offset, uint16_t value);

uint32_t PCI_ReadBAR(PCI_DeviceInfo* info, uint8_t BarOffset);