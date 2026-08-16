// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>
#include <arch/x86_64/std/stdint.hpp>

#include <arch/x86_64/ACPI/timer_defs.hpp>

#include <arch/x86_64/ACPI/acpi.hpp>
#include <arch/x86_64/ACPI/apic.hpp>

namespace krnl
{
	constexpr uint8_t HPET_CONV_SHIFT = 48;

	enum HPET_TimerConfigAndCapabilities
	{
		HPET_TIMER_CFG_INT_TYPE_MASK = (1ULL << 1),
		HPET_TIMER_CFG_INT_ENABLE_MASK = (1ULL << 2),
		HPET_TIMER_CFG_TYPE_MASK = (1ULL << 3),
		HPET_TIMER_CFG_PERIODIC_CAP_MASK = (1ULL << 4),
		HPET_TIMER_CFG_SIZE_CAP_MASK = (1ULL << 5),
		HPET_TIMER_CFG_PERIODIC_ACCUM_MASK = (1ULL << 6),
		HPET_TIMER_CFG_INT_ROUTING_MASK = (0x1FULL << 9),
		HPET_TIMER_CFG_INT_ROUTING_SHIFT = 9,
		HPET_TIMER_CFG_FSB_ENABLE_MASK = (1ULL << 14),
		HPET_TIMER_CFG_FSB_CAP_MASK = (1ULL << 15),
		HPET_TIMER_CFG_ROUTE_CAP_MASK = 0xFFFFFFFF00000000ULL,
		HPET_TIMER_CFG_ROUTE_CAP_SHIFT = 32
	};

	struct PACK HPET_TimerConfig
	{
		uint64_t ConfigAndCap;
		uint64_t ComparatorValueRegister;
		uint64_t FSBInterruptRouteRegister;
	};

	enum HPET_Capabilities : uint64_t
	{
		HPET_CAP_REVISION_ID_MASK = 0xFFULL,
		HPET_CAP_TIMER_COUNT_MASK = 0x1F00ULL,
		HPET_CAP_TIMER_COUNT_SHIFT = 8,
		HPET_CAP_COUNTER_SIZE_MASK = 0x2000ULL,
		HPET_CAP_LEGACY_REPLACEMENT_MASK = 0x8000ULL,
		HPET_CAP_VENDOR_ID_MASK = 0xFFFF0000ULL,
		HPET_CAP_VENDOR_ID_SHIFT = 16,
		HPET_CAP_CLOCK_PERIOD_MASK = 0xFFFFFFFF00000000ULL,
		HPET_CAP_CLOCK_PERIOD_SHIFT = 32,
	};

	enum HPET_Config : uint64_t
	{
		HPET_CONFIG_ENABLE_MASK = 0x1ULL,
		HPET_CONFIG_LEGACY_REPLACE_MASK = 0x2ULL,
	};

	enum HPET_InterruptStatus : uint64_t
	{
		HPET_CONFIG_INTERRUPT_STATUS_MASK = 0xFFFFFFFFULL,
	};

	struct PACK HPET_Registers
	{
		uint64_t generalCapAndID;
		uint64_t _Reserved;
		uint64_t generalConfig;
		uint64_t _Reserved1;
		uint64_t generalInterruptStatus;
		uint64_t _Reserved2[25];
		uint64_t mainCounterValue;
		uint64_t _Reserved3;

		HPET_TimerConfig timer0;
		uint64_t _Reserved4;

		HPET_TimerConfig timer1;
		uint64_t _Reserved5;

		HPET_TimerConfig timer2;
		uint64_t _Reserved6;

		HPET_TimerConfig timers3_31[28];
	};

	struct HPET_Device
	{
		APIC* apic;
		ACPI_HPET* hpet;
		volatile HPET_Registers* regs;
		size_t timerCount;
		uint64_t frequency;
		uint16_t vendorID;
		bool is64bitCapable;
		uint8_t lastUsedTimer;
		uint64_t ticksToNsMultiplier;
		uint64_t nsToTicksMultiplier;
	};

	struct HPET_TimerDevice
	{
		volatile HPET_TimerConfig* config;
		uint8_t irqLine;
		uint8_t timerIndex;
	};

	class HPET_Timer
	{
	public:
		bool Initialize(HPET_Device* device);

		bool SetupTimer(HPET_Device* device, HPET_TimerDevice* timerOut, uint8_t* isrOut, bool periodic);
		bool ArmTimer(HPET_Device* device, HPET_TimerDevice* timer, uint64_t targetNs, uint64_t counterBase = 0);
		bool StopTimer(HPET_Device* device, HPET_TimerDevice* timer);

		bool ResetMainCounter(HPET_Device* device);
		bool StartMainCounter(HPET_Device* device);
		bool StopMainCounter(HPET_Device* device);
		bool RestartMainCounter(HPET_Device* device);

		uint64_t GetCounterTicks(HPET_Device* device);
		uint64_t GetCounterFrequency(HPET_Device* device);

		uint64_t GetNSSinceStartup(HPET_Device* device);
	};
}