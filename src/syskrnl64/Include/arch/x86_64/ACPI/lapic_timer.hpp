// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>

#include <const_array.hpp>

#include <arch/x86_64/MP/smp_defs.hpp>
#include <arch/x86_64/MP/lpdata.hpp>

#include <arch/x86_64/ACPI/apic.hpp>

#include <arch/x86_64/ACPI/timer_defs.hpp>
#include <arch/x86_64/ACPI/hpet.hpp>

#include <arch/x86_64/Interrupts/isr.hpp>

namespace krnl
{
	enum class LT_Mode : uint8_t
	{
		None = 0,
		Periodic,
		OneShot,
		TSCDeadline
	};

	enum LT_CPUDescFlags : uint8_t
	{
		// bit 0: running? 0: no, 1: yes
		LT_FLAG_RUNNING = (1 << 0),

		// bit 1: TSC Deadline supported? 0: no, 1: yes
		LT_FLAG_TSC_DEADLINE_SUPPORTED = (1 << 1),
	};

	struct LT_CPUDesc
	{
		uint64_t frequency;
		uint64_t intervalNS;
		TimerEventHandler eventHandler;

		LT_Mode currentMode;
		uint8_t interruptVector;
		uint8_t divideConfig;
		uint8_t flags;
	};

	constexpr uint32_t LAPIC_LVT_TIMER = 0x320;
	constexpr uint32_t LAPIC_TIMER_INIT_COUNT = 0x380;
	constexpr uint32_t LAPIC_TIMER_CURRENT_COUNT = 0x390;
	constexpr uint32_t LAPIC_TIMER_DIVIDE = 0x3E0;

	class LAPIC_Timer
	{
	public:
		bool Initialize(APIC* apic, ISR* isr, LPData* lpdata);
		bool InitCPU(LPID cpuID);
		bool Calibrate(LPID cpuID, uint64_t frequency);

		bool SetMode(LPID cpuID, LT_Mode mode);
		bool SetIntervalNS(LPID cpuID, uint64_t nanoseconds);
		bool SetEvent(LPID cpuID, TimerEventHandler handler);

		constexpr uint64_t GetFrequency(LPID cpuID) { return cpuDescs[cpuID].frequency;}
		uint32_t GetCounterValue(LPID cpuID);

		bool Start(LPID cpuID, uint64_t initialNS);
		bool Stop(LPID cpuID);
	private:
		APIC* apic;
		ISR* isr;
		LPData* lpdata;
		stdEx::const_array<LT_CPUDesc> cpuDescs;

		static LAPIC_Timer* instance;

		static void InterruptHandler(ISR_InterruptStackFrame* regs);
	};
}