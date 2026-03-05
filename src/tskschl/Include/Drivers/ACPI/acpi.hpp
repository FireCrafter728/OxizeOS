#pragma once

#include <stdint.hpp>
#include <SysTable.hpp>

#ifndef PACK
#define PACK __attribute__((packed))
#endif

namespace TskSchl
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

        class ACPI
        {
        public:
            ACPI() = default;
            ACPI(SystemTable* System);
            bool Initialize(SystemTable* System);
            MCFG* GetMCFG();
        private:
            RSDP* rsdp;
            XSDT* xsdt;
            size_t xsdtEntries;
            MCFG* mcfg;
            static constexpr char RSDPSignature[8] = {'R', 'S', 'D', ' ', 'P', 'T', 'R', ' '};
            static constexpr char XSDTSignature[4] = {'X', 'S', 'D', 'T'};
            static constexpr char MCFGSignature[4] = {'M', 'C', 'F', 'G'};
        };
    }
}