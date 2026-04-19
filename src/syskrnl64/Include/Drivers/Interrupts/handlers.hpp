#pragma once

#include <Drivers/Interrupts/irq.hpp>

namespace SysKrnl64
{
    namespace IntHandlers
    {
        extern size_t ticks;
        class IntHandlers
        {
        public:
            IntHandlers() = default;
            IntHandlers(ISR::ISR* isr, IRQ::IRQ* irq);
            bool Initialize(ISR::ISR* isr, IRQ::IRQ* irq);
        private:
            static void SpuriousInterruptHandler(ISR::Registers* regs);
            static void PITHandler(ISR::Registers* regs);
            static void KeyboardHandler(ISR::Registers* regs);
        };
    }
}