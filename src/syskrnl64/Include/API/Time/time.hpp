// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>

#include <arch/x86_64/ACPI/timer.hpp>

namespace API
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
		KRNL_STATUS Initialize(krnl::Timer* timer);
		FormattedTime GetTimeFormatted();
		FormattedDate GetDateFormatted();
		TimeDate GetTimeDate();
		uint64_t GetMillisSinceEpoch();
		uint64_t GetNanoSinceEpoch();
	private:
		krnl::Timer* timer;
	};
}