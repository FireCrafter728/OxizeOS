#pragma once
#include <DISK/SATA_AHCI.hpp>

namespace x86Kernel
{
    namespace DISK
    {
        struct DiskParams
        {
            SATA_AHCI::Device device;
        };

        class DISK
        {
        public:
            DISK() = default;
            explicit DISK(DiskParams* params);
            bool Initialize(DiskParams* params);
            bool ReadSectors(uint32_t lba, uint16_t count, void* dataOut);
            bool WriteSectors(uint32_t lba, uint16_t count, const void* buffer);
            ~DISK() = default;
        private:
            DiskParams* params = nullptr;
        };
    }
}