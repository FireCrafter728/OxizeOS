// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |----------------------------------------------------------------------| //
// | OxizeOS Kernel Implementation                                        | //
// | Timer: A driver for abstracting and setting up various timer devices | //
// |----------------------------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdint.hpp>

#include <Drivers/APIC/apic.hpp>

#include <Drivers/Interrupts/isr.hpp>
#include <Drivers/Interrupts/irq.hpp>

#include <Drivers/Timer/timer_defs.hpp>
#include <Drivers/Timer/hpet.hpp>
#include <Drivers/Timer/lapic_timer.hpp>
#include <Drivers/Timer/itsc.hpp>

namespace SysKrnl64
{
    namespace Timer
    {
        constexpr uint64_t SLEEP_BUSYWAIT_TRESHOLD = NS_PER_MICROSECOND * 100;

        struct TimerDesc
        {
            SystemTable* System;
            IRQ::IRQ* irq;
            HPET_Timer* hpet;
            HPETDevice* hpetDevice;
        };

        enum class PrimaryTimer : uint8_t
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

            PrimaryTimer primaryTimer;
            
            uint8_t hpetIsr;
            HPETTimer hpetTimer = {};
            static void HPET_InterruptHandler(ISR::Registers* regs);
            TimerEventHandler hpetEvent;
            SystemTime initTime;
        };
    }
}