// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>

#include <arch/x86_64/ACPI/apic.hpp>
#include <arch/x86_64/ACPI/hpet.hpp>
#include <arch/x86_64/ACPI/lapic_timer.hpp>

#include <arch/x86_64/Interrupts/isr.hpp>
#include <arch/x86_64/Interrupts/irq.hpp>

#include <arch/x86_64/Utility/itsc.hpp>

#include <arch/x86_64/ACPI/timer_defs.hpp>

namespace krnl
{
	constexpr uint64_t TIMER_SLEEP_BUSYWAIT_TRESHOLD = NS_PER_MICROSECOND * 100;

	struct TimerDesc
	{
		SystemTable* System;
		IRQ* irq;
		HPET_Timer* hpet;
		HPET_Device* hpetDevice;
	};

	enum class Timer_PrimaryTimer : uint8_t
	{
		HPET,
		iTSC,
	};

	class Timer
	{
	public:
		bool Initialize(const TimerDesc* desc);

		SystemTime GetSystemTime();

		void SleepMS(uint64_t milliseconds);
		void SleepNS(uint64_t nanoseconds);
	private:
		static Timer* instance;
		const TimerDesc* desc;

		Timer_PrimaryTimer primaryTimer;
		
		uint8_t hpetIsr;
		HPET_TimerDevice hpetTimer = {};
		static void HPET_InterruptHandler(ISR_InterruptStackFrame* regs);
		TimerEventHandler hpetEvent;
		SystemTime initTime;
	};
}