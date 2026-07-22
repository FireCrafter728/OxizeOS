// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |-------------------------------------------------------------------------------------------------| //
// | OxizeOS Kernel Implementation                                                                   | //
// | LAPIC Timer: A driver for managing the lapic timer and exposing various timer-related functions | //
// |-------------------------------------------------------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdint.hpp>
#include <const_array.hpp>

#include <Drivers/MP/smp_defs.hpp>
#include <Drivers/MP/lcpu.hpp>

#include <Drivers/APIC/apic.hpp>

#include <Drivers/Timer/timer_defs.hpp>
#include <Drivers/Timer/hpet.hpp>

#include <Drivers/Interrupts/isr.hpp>

namespace SysKrnl64
{
    namespace Timer
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
            bool Initialize(APIC::APIC* apic, ISR::ISR* isr, MP::LCPU* lcpu);
            bool InitCPU(MP::LPID cpuID);
            bool Calibrate(MP::LPID cpuID, uint64_t frequency);

            bool SetMode(MP::LPID cpuID, LT_Mode mode);
            bool SetIntervalNS(MP::LPID cpuID, uint64_t nanoseconds);
            bool SetEvent(MP::LPID cpuID, TimerEventHandler handler);

            constexpr uint64_t GetFrequency(MP::LPID cpuID) { return cpuDescs[cpuID].frequency;}
            uint32_t GetCounterValue(MP::LPID cpuID);

            bool Start(MP::LPID cpuID, uint64_t initialNS);
            bool Stop(MP::LPID cpuID);
        private:
            APIC::APIC* apic;
            ISR::ISR* isr;
            MP::LCPU* lcpu;
            stdEx::const_array<LT_CPUDesc> cpuDescs;

            static LAPIC_Timer* instance;

            static void InterruptHandler(ISR::Registers* regs);
        };
    }
}