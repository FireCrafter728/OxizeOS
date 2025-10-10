#pragma once

namespace Bootloader
{
    namespace ACPI
    {
        #pragma pack(push, 1)
        struct RSDPDescV1
        {
            char Signature[8];
            uint8_t Checksum;
            char OEMId[6];
            uint8_t Revision;
            uint32_t RSDTAddr;
        };

        struct RSDPDescV2
        {
            RSDPDescV1 v1;
            uint32_t Length;
            uint64_t XsdtAddress;
            uint8_t ExtendedChecksum;
            uint8_t _Reserved[3];
        };

        struct ACPISDTHeader
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

        struct XSDT
        {
            ACPISDTHeader header;
            uint64_t Ptr[];
        };

        struct MCFGEntry
        {
            uint64_t baseAddr;
            uint16_t segmentGroup;
            uint8_t startBus;
            uint8_t endBus;
            uint32_t Reserved;
        };

        struct MCFG
        {
            ACPISDTHeader header;
            uint64_t Reserved = 0;
            MCFGEntry* entries = nullptr;
        };

        #pragma pack(pop)

        struct ACPIDesc
        {
            const RSDPDescV2* rsdp = nullptr;
            const XSDT* xsdt = nullptr;
        };

        class ACPI
        {
        public:
            ACPI() = default;
            ACPI(ACPIDesc* desc, EFI_CONFIGURATION_TABLE* confTables, size_t tableCount);
            bool Initialize(ACPIDesc* desc, EFI_CONFIGURATION_TABLE* confTables, size_t tableCount);
            bool GetMCFG(MCFG* mcfg);
        private:
            ACPIDesc* desc;
            const ACPISDTHeader* FindTable(const char* signature);
        };
    }
}