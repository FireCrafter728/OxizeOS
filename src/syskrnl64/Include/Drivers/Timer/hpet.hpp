// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |------------------------------------------------------------| //
// | OxizeOS Kernel Implementation                              | //
// | HPET: A driver for managing the High Precision Event Timer | //
// |------------------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdint.hpp>

#include <Drivers/Timer/timer_defs.hpp>

#include <Drivers/ACPI/acpi.hpp>

#ifndef PACK
#define PACK __attribute__((packed))
#endif

namespace SysKrnl64
{
    namespace Timer
    {
        constexpr uint8_t HPET_CONV_SHIFT = 48;

        enum HPETTimerConfigAndCapabilities
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

        struct PACK HPETTimerConfig
        {
            uint64_t ConfigAndCap;
            uint64_t ComparatorValueRegister;
            uint64_t FSBInterruptRouteRegister;
        };

        enum HPETCapabilities : uint64_t
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

        enum HPETConfig : uint64_t
        {
            HPET_CONFIG_ENABLE_MASK = 0x1ULL,
            HPET_CONFIG_LEGACY_REPLACE_MASK = 0x2ULL,
        };

        enum HPETInterruptStatus : uint64_t
        {
            HPET_CONFIG_INTERRUPT_STATUS_MASK = 0xFFFFFFFFULL,
        };

        struct PACK HPETRegisters
        {
            uint64_t generalCapAndID;
            uint64_t _Reserved;
            uint64_t generalConfig;
            uint64_t _Reserved1;
            uint64_t generalInterruptStatus;
            uint64_t _Reserved2[25];
            uint64_t mainCounterValue;
            uint64_t _Reserved3;

            HPETTimerConfig timer0;
            uint64_t _Reserved4;

            HPETTimerConfig timer1;
            uint64_t _Reserved5;

            HPETTimerConfig timer2;
            uint64_t _Reserved6;

            HPETTimerConfig timers3_31[28];
        };

        struct HPETDevice
        {
            APIC::APIC* apic;
            ACPI::HPET* hpet;
            HPETRegisters* regs;
            size_t timerCount;
            uint64_t frequency;
            uint16_t vendorID;
            bool is64bitCapable;
            uint8_t lastUsedTimer;
            uint64_t ticksToNsMultiplier;
            uint64_t nsToTicksMultiplier;
        };

        struct HPETTimer
        {
            HPETTimerConfig* config;
            uint8_t irqLine;
            uint8_t timerIndex;
        };

        class HPET_Timer
        {
        public:
            bool Initialize(HPETDevice* device);

            bool SetupTimer(HPETDevice* device, HPETTimer* timerOut, uint8_t* isrOut, bool periodic);
            bool ArmTimer(HPETDevice* device, HPETTimer* timer, uint64_t targetNs, uint64_t counterBase = 0);
            bool StopTimer(HPETDevice* device, HPETTimer* timer);

            bool ResetMainCounter(HPETDevice* device);
            bool StartMainCounter(HPETDevice* device);
            bool StopMainCounter(HPETDevice* device);
            bool RestartMainCounter(HPETDevice* device);

            uint64_t GetCounterTicks(HPETDevice* device);
            uint64_t GetCounterFrequency(HPETDevice* device);

            uint64_t GetNSSinceStartup(HPETDevice* device);
        };
    }
}