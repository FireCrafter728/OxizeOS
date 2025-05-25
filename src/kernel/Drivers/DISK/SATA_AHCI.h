#pragma once
#include <PCI/pci.h>

#define SATA_AHCI_USED_PORT_COUNT   8

#define SATA_AHCI_MAX_PRDT_ENTRIES  8

#define SATA_AHCI_COMMAND_SLOTS     32

#define SECTOR_SIZE                 512

#define SATA_AHCI_MASS_STORAGE_CONTROLLER 0x01
#define SATA_AHCI_SATA_CONTROLLER 0x06
#define SATA_AHCI_1_0_CONTROLLER 0x01

typedef struct
{
    // DWORD 0
    uint8_t CommandFISLength : 5;        // Command FIS Length (in DWORDs)
    uint8_t ATAPICommand : 1;            // 1 = ATAPI device
    uint8_t WriteDirection : 1;          // 1 = Write to device, 0 = Read from device
    uint8_t Prefetchable : 1;            // 1 = Prefetchable

    uint8_t Reset : 1;                   // 1 = Reset
    uint8_t BIST : 1;                    // 1 = BIST request
    uint8_t ClearBusyUponOK : 1;         // 1 = Clear busy upon receiving R_OK
    uint8_t _Reserved0 : 1;
    uint8_t PortMultiplier : 4;          // Port multiplier port

    uint16_t PRDTLength;                 // Physical Region Descriptor Table Length (in entries)

    // DWORD 1
    volatile uint32_t PRDByteCountTransferred; // Bytes transferred by PRDT entries

    // DWORD 2–3
    uint32_t CommandTableBaseAddressLow;  // Lower 32 bits of Command Table base address
    uint32_t CommandTableBaseAddressHigh; // Upper 32 bits of Command Table base address

    // DWORD 4–7
    uint32_t _Reserved1[4];
} __attribute__((packed)) SATA_AHCI_CommandHeader;

typedef struct
{
    uint8_t fisType;
    uint8_t portMultiplier : 4;
    uint8_t reservedBits0 : 3;
    uint8_t commandControlBit : 1;
    uint8_t ataCommandCode;
    uint8_t featuresLow;
    uint8_t LBALow0;
    uint8_t LBALow1;
    uint8_t LBALow2;
    uint8_t deviceRegister;
    uint8_t LBAHigh3;
    uint8_t LBAHigh4;
    uint8_t LBAHigh5;
    uint8_t featuresHigh;
    uint8_t sectorCountLow;
    uint8_t sectorCountHigh;
    uint8_t isoCommandCompletion;
    uint8_t controlRegister;
    uint8_t reservedBytes[4];
} __attribute__((packed)) SATA_AHCI_COMMAND_FIS;

typedef struct
{
    // Command FIS: 64 bytes
    uint8_t CommandFIS[64];

    // ATAPI Command: 16 bytes
    uint8_t ATAPICommand[16];

    // Reserved: 48 bytes
    uint8_t _Reserved[48];

    // Physical Region Descriptor Table entries
    struct SATA_AHCI_PRDTEntry
    {
        uint32_t DataBaseAddressLow;        // Data base address (lower 32 bits)
        uint32_t DataBaseAddressHigh;       // Data base address (upper 32 bits)
        uint32_t _Reserved0;                  // Reserved
        uint32_t ByteCountAndInterrupt;     // Byte count (22 bits) and Interrupt on Completion (bit 31)
    } __attribute__((packed)) PRDTEntries[SATA_AHCI_MAX_PRDT_ENTRIES];
} __attribute__((packed)) SATA_AHCI_CommandTable;

typedef struct
{
    volatile uint32_t* port_mmio;
    uint8_t port_number;
    bool deviceActive;
    SATA_AHCI_CommandHeader* commandHeaders;
    SATA_AHCI_CommandTable* commandTables[SATA_AHCI_COMMAND_SLOTS];
} __attribute__((packed)) SATA_AHCI_Port;

typedef struct {
    uint8_t model[41];
    uint32_t sectorCount;
} __attribute__((packed)) SATA_AHCI_DriveInfo;

typedef struct
{
    PCI_DeviceInfo info;
    volatile uint32_t* AHCI_BAR;
    SATA_AHCI_Port AHCIPorts[SATA_AHCI_USED_PORT_COUNT];
    char IdentifyData[SECTOR_SIZE];
    SATA_AHCI_DriveInfo driveInfo;
} SATA_AHCI_Device;

bool SATA_AHCI_Initialize(SATA_AHCI_Device* device);
bool SATA_AHCI_ReadSectors(SATA_AHCI_Device* device, uint32_t lba, uint8_t count, void* dataOut);