#pragma once
#include <stdint.h>
#include <stddef.h>

namespace x86Kernel
{
    namespace ACPI
    {
        struct __attribute__((packed)) RSDPDescV1
        {
            char Signature[8];
            uint8_t Checksum;
            char OEMId[6];
            uint8_t Revision;
            uint32_t RSDTAddr;
        };

        struct __attribute__((packed)) RSDPDescV2
        {
            RSDPDescV1 v1;
            uint32_t Length;
            uint64_t XsdtAddress;
            uint8_t ExtendedChecksum;
            uint8_t _Reserved[3];
        };

        struct __attribute__((packed)) ACPISDTHeader
        {
            char Signature[4];
            uint32_t Length;
            uint8_t Revision;
            uint8_t Checksum;
            char OEMId[6];
            char OEMTableId[8];
            uint32_t OEMRevision;
            uint32_t CreatorID;
            uint32_t CreatorRevision;
        };

        struct __attribute__((packed)) RSDT
        {
            ACPISDTHeader header;
            uint32_t Ptr[];
        };

        struct __attribute__((packed)) XSDT
        {
            ACPISDTHeader header;
            uint64_t Ptr[];
        };

        struct __attribute__((packed)) MADT
        {
            ACPISDTHeader header;
            uint32_t LocalAPICAddr;
            uint32_t Flags;
        };

        struct __attribute__((packed)) MADTEntryHeader
        {
            uint8_t Type;
            uint8_t Length;
        };

        enum class MADTEntryType : uint8_t
        {
            LocalAPIC = 0,
            IOAPIC = 1,
            InterruptSourceOverride = 2,
            NMI = 3,
            LocalAPIC_NMI = 4,
            LocalAPIC_AddrOverride = 5,
        };

        struct __attribute__((packed)) MADTLocalAPIC
        {
            MADTEntryHeader header;
            uint8_t ACPIProcessorID;
            uint8_t APICID;
            uint32_t Flags;
        };

        struct __attribute__((packed)) MADTIOAPIC
        {
            MADTEntryHeader header;
            uint8_t IOAPICID;
            uint8_t _Reserved;
            uint32_t IOAPICAddr;
            uint32_t GlobalSystemInterruptBase;
        };

        struct __attribute__((packed)) MADTInterruptSourceOverride
        {
            MADTEntryHeader header;
            uint8_t BusSource;
            uint8_t IRQSource;
            uint32_t GlobalSystemInterrupt;
            uint16_t Flags;
        };

        struct __attribute__((packed)) MADTLocalAPICNMI
        {
            MADTEntryHeader header;
            uint8_t ACPIProcessorID;
            uint16_t Flags;
            uint8_t LINTNumber;
        };

        struct MADTDeviceList
        {
            MADTLocalAPIC lapics[32];
            size_t lapicCount = 0;

            MADTIOAPIC ioapics[8];
            size_t ioapicCount = 0;

            MADTInterruptSourceOverride overrides[16];
            size_t overrideCount = 0;
        };

        struct ACPIDesc
        {
            const RSDPDescV2 *rsdp = nullptr;
            const ACPISDTHeader *rsdt = nullptr;
            const XSDT *xsdt = nullptr;
            const MADT *madt = nullptr;
            MADTDeviceList deviceList;
        };

        class ACPI
        {
        public:
            ACPI() = default;
            ACPI(ACPIDesc *desc);
            bool Initialize(ACPIDesc *desc);
            ~ACPI() = default;
            bool inline IsInitialized() const { return Initialized; };
        private:
            ACPIDesc *desc;
            const RSDPDescV2 *GetRSDPDesc();
            const ACPISDTHeader *FindTable(const char *signature);
            bool ParseMADT(MADTDeviceList *list);
            bool Initialized = false;
        };
    }
}