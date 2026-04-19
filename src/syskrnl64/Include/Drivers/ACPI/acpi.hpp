#pragma once

#include <stdint.hpp>
#include <SysTable.hpp>

#ifndef PACK
#define PACK __attribute__((packed))
#endif

namespace SysKrnl64
{
    namespace ACPI
    {
        struct PACK RSDP
        {
            // RSDP
            uint8_t Signature[8]; // "RSD PTR "
            uint8_t Checksum;
            uint8_t OEMID[6];
            uint8_t Revision;
            uint32_t RsdtAddress;

            // XSDP Extension, in this case, ALWAYS present
            uint32_t Length;
            uint64_t XsdtAddress;
            uint8_t ExtendedChecksum;
            uint8_t reserved[3];
        };

        struct PACK ACPISDTHeader
        {
            uint8_t Signature[4];
            uint32_t Length;
            uint8_t Revision;
            uint8_t Checksum;
            uint8_t OEMID[6];
            uint8_t OEMTableID[8];
            uint32_t OEMRevision;
            uint32_t CreatorID;
            uint32_t CreatorRevision;
        };

        struct PACK XSDT
        {
            ACPISDTHeader sdt;
            uintptr_t entries[];
        };

        struct PACK MCFGEntry
        {
            uint64_t BaseAddr;
            uint16_t SegmentGroup;
            uint8_t StartBus;
            uint8_t EndBus;
            uint32_t Reserved;
        };

        struct PACK MCFG
        {
            ACPISDTHeader sdt;
            uint64_t Reserved;
            MCFGEntry entries[];
        };

        struct PACK MADT
        {
            ACPISDTHeader sdt;
            uint32_t lapicAddr;
            uint32_t flags;
        };

        struct PACK MADTEntryHeader
        {
            uint8_t type, length;   
        };  

        struct PACK MADT_LAPIC
        {
            MADTEntryHeader hdr;
            uint8_t apiccpuid, apicid;
            uint32_t flags;
        };

        struct PACK MADT_IOAPIC
        {
            MADTEntryHeader hdr;
            uint8_t ioapicId, reserved;
            uint32_t ioapicAddr;
            uint32_t globalSysInterruptBase;
        };

        struct PACK MADT_ISO
        {
            MADTEntryHeader hdr;
            uint8_t busSource, IRQSource;
            uint32_t globalSysIntr;
            uint16_t flags;
        };

        struct PACK MADT_IOAPIC_NMI
        {
            MADTEntryHeader hdr;
            uint8_t nmiSource, reserved;
            uint16_t flags;
            uint32_t gsi;
        };

        struct PACK MADT_LAPIC_NMI
        {
            MADTEntryHeader hdr;
            uint8_t acpicpuid;
            uint16_t flags;
            uint8_t lint;
        };

        struct PACK MADT_LAPIC_ADDR_OVERRIDE
        {
            MADTEntryHeader hdr;
            uint16_t reserved;
            uint64_t lapicAddr;
        };

        struct PACK MADT_X2APIC
        {
            MADTEntryHeader hdr;
            uint16_t reserved;
            uint32_t x2apicId, flags, apicProcID;
        };

        struct PACK MADT_X2APIC_NMI
        {
            MADTEntryHeader hdr;
            uint16_t flags;
            uint32_t apicProcID;
            uint8_t lint, reserved[3];
        };

        class ACPI
        {
        public:
            ACPI() = default;
            ACPI(SystemTable* System);
            bool Initialize(SystemTable* System);
            MCFG* GetMCFG();
            MADT* GetMADT();
        private:
            RSDP* rsdp;
            XSDT* xsdt;
            size_t xsdtEntries;
            MCFG* mcfg;
            MADT* madt;
            static constexpr char RSDPSignature[8] = {'R', 'S', 'D', ' ', 'P', 'T', 'R', ' '};
            static constexpr char XSDTSignature[4] = {'X', 'S', 'D', 'T'};
            static constexpr char MCFGSignature[4] = {'M', 'C', 'F', 'G'};
            static constexpr char MADTSignature[4] = {'A', 'P', 'I', 'C'};
        };
    }
}