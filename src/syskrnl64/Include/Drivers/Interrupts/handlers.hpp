// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |-----------------------------------------------------------| //
// | OxizeOS Kernel Implementation                             | //
// | INTERRUPT HANDLERS: Driver for storing interrupt handlers | //
// |-----------------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <Drivers/Interrupts/irq.hpp>

namespace SysKrnl64
{
    namespace IntHandlers
    {
        class IntHandlers
        {
        public:
            IntHandlers() = default;
            IntHandlers(ISR::ISR* isr, IRQ::IRQ* irq);
            bool Initialize(ISR::ISR* isr, IRQ::IRQ* irq);
        private:
            // APIC IRQs
            static void SpuriousInterruptHandler(ISR::Registers* regs);

            // CPU Exceptions

            static void DoubleFaultHandler(ISR::Registers* regs);
            static void NonMaskableInterruptHandler(ISR::Registers* regs);
            static void MachineCheckExceptionHandler(ISR::Registers* regs); 
        };
    }
}