// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// ||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |---------------------------------------------------| //
// | OxizeOS Kernel Implementation                     | //
// | timer_defs: Various definitions for timer drivers | //
// |---------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||||| //

namespace SysKrnl64
{
    namespace Timer
    {
        typedef void (*TimerEventHandler)(ISR::Registers* regs);

        constexpr uint64_t FS_PER_SECOND = 1'000'000'000'000'000ULL;
        constexpr uint64_t NS_PER_SECOND = 1'000'000'000ULL;
        constexpr uint64_t NS_PER_MILLISECOND = 1'000'000ULL;
        constexpr uint64_t NS_PER_MICROSECOND = 1'000ULL;
    }
}