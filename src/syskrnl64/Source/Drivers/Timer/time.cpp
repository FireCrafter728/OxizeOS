// SPDX-License-Identifier: GPL-3.0-or-later

#include <Drivers/Timer/time.hpp>

using namespace SysKrnl64::Timer;

bool Time::Initialize(Timer* timer)
{
    if(!timer)
    {
        printf("[SYSKRNL64] [TIME] [ERROR]: Invalid Initialize() input params\r\n");
        return false;
    }

    this->timer = timer;

    return true;
}

FormattedTime Time::GetTimeFormatted()
{
    SystemTime sysTime = timer->GetSystemTime();

    FormattedTime result = {};

    uint64_t secondsToday = sysTime.SecondsSinceEpoch % 86400;

    result.hour = secondsToday / 3600;
    secondsToday %= 3600;

    result.minute = secondsToday / 60;
    result.second = secondsToday % 60;

    result.millisecond = sysTime.Nanoseconds / 1000000;
    result.nanosecond = sysTime.Nanoseconds;

    return result;
}

FormattedDate Time::GetDateFormatted()
{
    SystemTime sysTime = timer->GetSystemTime();

    FormattedDate result = {};

    uint64_t days = sysTime.SecondsSinceEpoch / 86400;
    
    result.year = 2000;
    
    auto isLeapYear = [](uint32_t year) -> bool
	{
		return (year % 4 == 0) && ((year % 100) != 0 || (year % 400) == 0);
	};

    while(true)
    {
        uint16_t daysInYear = isLeapYear(result.year) ? 366 : 365;
        if(days < daysInYear) break;
        days -= daysInYear;
        result.year++;
    }

    constexpr uint8_t DaysPerMonth[] =
    {
        31, 28, 31, 30,
        31, 30, 31, 31,
        30, 31, 30, 31
    };

    result.month = 1;

    while(true)
    {
        uint8_t daysInMonth = DaysPerMonth[result.month - 1];
        if(result.month == 2 && isLeapYear(result.year)) daysInMonth++;

        if(days < daysInMonth) break;

        days -= daysInMonth;
        result.month++;
    }

    result.day = static_cast<uint8_t>(days + 1);

    return result;
}

TimeDate Time::GetTimeDate()
{
    TimeDate result = {};
    result.time = GetTimeFormatted();
    result.date = GetDateFormatted();
    return result;
}

uint64_t Time::GetMillisSinceEpoch()
{
    SystemTime sysTime = timer->GetSystemTime();
    return sysTime.SecondsSinceEpoch * 1000 + sysTime.Nanoseconds / 1000000;
}

uint64_t Time::GetNanoSinceEpoch()
{
    SystemTime sysTime = timer->GetSystemTime();
    return sysTime.SecondsSinceEpoch * 1000000000 + sysTime.Nanoseconds;
}