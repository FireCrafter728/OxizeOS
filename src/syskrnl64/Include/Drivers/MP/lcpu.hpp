// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |----------------------------------------------------------------------------| //
// | OxizeOS Kernel Implementation                                              | //
// | Logical CPU: A driver for enumerating and managing logical processor cores | //
// |----------------------------------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdint.hpp>
#include <const_array.hpp>

#include <Drivers/MP/smp_defs.hpp>

#include <Drivers/APIC/apic.hpp>

namespace SysKrnl64
{
    namespace MP
    {
        class LCPU
        {
        public:
            bool Initialize(APIC::APIC* apic);
            inline LPSpecificData* GetLPDataByID(LPID lpid) { return &lpData[lpid]; }
            LPSpecificData* GetLPDataForCurrentLP();
            inline size_t GetLPCount() { return lpData.size(); }
        private:
            stdEx::const_array<LPSpecificData> lpData;
        };
    }
}