#pragma once
#include <PCI/pci.hpp>

#define SATA_AHCI_USED_PORT_COUNT 8

#define SATA_AHCI_MAX_PRDT_ENTRIES 8

#define SATA_AHCI_COMMAND_SLOTS 32

#define SECTOR_SIZE 512

#define SATA_AHCI_MASS_STORAGE_CONTROLLER 0x01
#define SATA_AHCI_SATA_CONTROLLER 0x06
#define SATA_AHCI_1_0_CONTROLLER 0x01

namespace x86Kernel
{
    namespace SATA_AHCI
    {
        struct __attribute__((packed)) CommandHeader
        {
            uint8_t CommandFISLength : 5;
            uint8_t ATAPICommand : 1;
            uint8_t WriteDirection : 1;
            uint8_t Prefetchable : 1;

            uint8_t Reset : 1;
            uint8_t BIST : 1;
            uint8_t ClearBusyUponOK : 1;
            uint8_t _Reserved0 : 1;
            uint8_t PortMultiplier : 4;

            uint16_t PRDTLength;

            volatile uint32_t PRDByteCountTransferred;

            uint32_t CommandTableBaseAddressLow;
            uint32_t CommandTableBaseAddressHigh;

            uint32_t _Reserved1[4];
        };

        struct __attribute__((packed)) COMMAND_FIS
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
        };

        struct __attribute__((packed)) CommandTable
        {
            uint8_t CommandFIS[64];

            uint8_t ATAPICommand[16];

            uint8_t _Reserved[48];

            struct PRDTEntry
            {
                uint32_t DataBaseAddressLow;
                uint32_t DataBaseAddressHigh;
                uint32_t _Reserved0;
                uint32_t ByteCountAndInterrupt;
            } __attribute__((packed)) PRDTEntries[SATA_AHCI_MAX_PRDT_ENTRIES];
        };

        struct __attribute__((packed)) Port
        {
            volatile uint32_t *port_mmio;
            uint8_t port_number;
            bool deviceActive;
            CommandHeader *commandHeaders;
            CommandTable *commandTables[SATA_AHCI_COMMAND_SLOTS];
        };

        struct __attribute__((packed)) DriveInfo
        {
            uint8_t model[41];
            uint32_t sectorCount;
        };

        struct Device
        {
            PCI::DeviceInfo info;
            volatile uint32_t *AHCI_BAR;
            Port AHCIPorts[SATA_AHCI_USED_PORT_COUNT];
            char IdentifyData[SECTOR_SIZE];
            DriveInfo driveInfo;
        };

        bool Initialize(Device *device);
        bool ReadSectors(Device *device, uint32_t lba, uint16_t count, void *dataOut);
        bool WriteSectors(Device* device, uint32_t lba, uint16_t count, const void* buffer);
    };
}