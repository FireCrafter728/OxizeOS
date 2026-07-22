// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |---------------------------------------------------------------------| //
// | OxizeOS Kernel Implementation                                       | //
// | SMP Defs: Definitions, Type declarations, structs and enums for SMP | //
// |---------------------------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdint.hpp>
#include <stddef.hpp>

namespace SysKrnl64
{
    namespace MP
    {
        typedef uint32_t LPID; // Global logical processor identifier

        constexpr LPID INVALID_LPID = 0xFFFFFFFF;

        enum LCPUState : uint16_t
        {
            LP_STATE_UNKNOWN = 0,
            LP_STATE_FIRMWARE_DISABLED = 1,
            LP_STATE_UNINITIALIZED = 2,
            LP_STATE_INITIALIZING = 3,
            LP_STATE_ONLINE = 4,
            LP_STATE_HALTED = 5,
        };

        struct LPIdentifier
        {
            // Core identifiers
            LPID lpid;
            uint32_t apicId;

            // Extra identifiers
            uint32_t packageId, coreId, threadId;
        };

        // Data stored in the GS segment, unique data for each LP
        struct LPSpecificData
        {
            LPSpecificData* self; // Used to get quick access of this structure
            LPIdentifier identity;
        };

        static_assert(offsetof(LPSpecificData, self) == 0);
    }
}