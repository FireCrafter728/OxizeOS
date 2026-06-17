#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |----------------------------------------------------| //
// | OxizeOS Kernel Implementation                      | //
// | MSR: Driver for accessing Model Specific Registers | //
// |----------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#define MSR_APIC_BASE 0x1B
#define MSR_X2APIC_BASE 0x800

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