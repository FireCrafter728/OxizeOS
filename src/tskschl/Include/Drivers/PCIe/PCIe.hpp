#pragma once

#include <stdint.hpp>

#include <Drivers/ACPI/acpi.hpp>

#ifndef PACK
#define PACK __attribute__((packed))
#endif

typedef uint32_t BAR; // Base Address Register

#define PCIE_MAX_BUSSES 256
#define PCIE_MAX_DEVICES 32
#define PCIE_MAX_FUNCTIONS 8

#define PCIE_ANY16 0xFFFF
#define PCIE_ANY8 0xFF

#define PCIE_BAR0 0x10
#define PCIE_BAR1 0x14
#define PCIE_BAR2 0x18
#define PCIE_BAR3 0x1C
#define PCIE_BAR4 0x20
#define PCIE_BAR5 0x24

#define PCIE_COMMAND_IO_SPACE               0x01
#define PCIE_COMMAND_MMIO_SPACE             0x02
#define PCIE_COMMAND_BUS_MASTER             0x04
#define PCIE_COMMAND_SPECIAL_CYCLES         0x08
#define PCIE_COMMAND_MEM_WRITE_INV          0x10
#define PCIE_COMMAND_VGA_PALLETE            0x20
#define PCIE_COMMAND_PARITY_ERROR           0x40
#define PCIE_COMMAND_WAIT_CYCLE             0x80
#define PCIE_COMMAND_SERR_ENABLE            0x100
#define PCIE_COMMAND_FAST_B2B               0x200

namespace TskSchl
{
    namespace PCIe
    {
        struct PACK ConfigBlock
        {
            uint16_t VendorID, DeviceID;
            uint16_t Command, Status;
            uint8_t RevisionID;
            uint8_t ProgIF, SubClass, ClassCode;
            uint8_t CacheLineSize, LatencyTimer, HeaderType, BIST;

            BAR BARs[6];

            uint32_t CardbusCISPtr;
            uint16_t SubsystemVendorID, SubsystemID;
            uint32_t ExpansionROMBase;

            uint8_t CapPointer;

            uint8_t Reserved[7];
            uint64_t Reserved2;

            uint8_t Extended[0xFC0];
        };

        struct PACK ECAMSegment
        {
            uintptr_t ECAMBase;
        };

        enum class BARTypes : uint8_t
        {
            NONE = 0,
            IO,
            MMIO,
        };

        enum class BARArchs : uint8_t
        {
            IA32,
            AMD64,
        };

        struct BARInfo
        {
            uint8_t BarOffset;
            uintptr_t BarAddr;
            BARTypes BarType;
            BARArchs BarArch;
            size_t BarLength;
        };

        struct DeviceInfo
        {
            ConfigBlock* root;
            ConfigBlock* ExtraFunctions;
            uint8_t Bus, Device;
            uint16_t VendorID, DeviceID; // Duplication for simplicity and faster access, must mirror the actual data inside config block
            uint8_t progIF, subClass, classCode;
            BARInfo BARs[6];
        };

        class PCIe
        {
        public:
            PCIe() = default;
            PCIe(ACPI::MCFG* mcfg);
            bool Initialize(ACPI::MCFG* mcfg);
            bool LocateDevice(DeviceInfo* filters, DeviceInfo* infoOut);
            bool GetDeviceInfo(uint8_t bus, uint8_t device, DeviceInfo* infoOut);
        private:
            bool GetBARInfo(DeviceInfo* device, BARInfo* info, uint8_t function = 0);
            bool FilterOut(const DeviceInfo* filters, DeviceInfo* cmp);
            ACPI::MCFG* mcfg;
            ECAMSegment* Segments;
            size_t BussesImplemented;
            static constexpr DeviceInfo PCIBridgeFilter = {nullptr, nullptr, 0, 0, PCIE_ANY16, PCIE_ANY16, 0x00, 0x04, 0x06, {0}};
        };
    }
}