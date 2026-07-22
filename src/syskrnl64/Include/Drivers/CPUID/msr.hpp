// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |----------------------------------------------------| //
// | OxizeOS Kernel Implementation                      | //
// | MSR: Driver for accessing Model Specific Registers | //
// |----------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#define MSR_APIC_BASE 0x1B
#define X2APIC_MSR_BASE 0x800
#define MSR_IA32_TSC_DEADLINE 0x6E0
#define MSR_IA32_GS_BASE 0xC0000101

namespace SysKrnl64
{
    namespace MSR
    {
        class MSR
        {
        public:
            MSR() = default;
            bool Initialize();
            uint64_t ReadMSR(uint32_t msr);
            void WriteMSR(uint32_t msr, uint64_t value);
        private:
            bool present = false;
        };
    }
}