// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// ||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |-------------------------------------------------| //
// | OxizeOS Kernel Implementation                   | //
// | Time: Exposes the current formatted system time | //
// |-------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <Drivers/Timer/timer.hpp>

namespace SysKrnl64
{
    namespace Timer
    {
        struct FormattedTime
        {
            uint8_t hour, minute, second;
            uint32_t millisecond, nanosecond;
        };

        struct FormattedDate
        {
            uint16_t year;
            uint8_t month, day;
        };

        struct TimeDate
        {
            FormattedDate date;
            FormattedTime time;
        };

        class Time
        {
        public:
            bool Initialize(Timer* timer);
            FormattedTime GetTimeFormatted();
            FormattedDate GetDateFormatted();
            TimeDate GetTimeDate();
            uint64_t GetMillisSinceEpoch();
            uint64_t GetNanoSinceEpoch();
        private:
            Timer* timer;
        };
    }
}