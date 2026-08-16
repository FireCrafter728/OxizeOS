// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Time API Definitions

#include <main/defs.hpp>
#include <stdint.h>

#include <arch/x86_64/Interrupts/isr.hpp>

namespace krnl
{
	typedef void (*TimerEventHandler)(ISR_InterruptStackFrame* regs);

	constexpr uint64_t FS_PER_SECOND = 1'000'000'000'000'000ULL;
	constexpr uint64_t NS_PER_SECOND = 1'000'000'000ULL;
	constexpr uint64_t NS_PER_MILLISECOND = 1'000'000ULL;
	constexpr uint64_t NS_PER_MICROSECOND = 1'000ULL;
}