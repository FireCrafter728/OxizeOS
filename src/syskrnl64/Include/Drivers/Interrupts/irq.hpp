#pragma once

#include <Drivers/APIC/apic.hpp>
#include <Drivers/Interrupts/isr.hpp>

namespace SysKrnl64
{
    namespace IRQ
    {
        constexpr uint8_t MAX_IRQ_HANDLERS = 96;

        typedef void (*IRQHandler)(ISR::Registers* regs);

        class IRQ
        {
        public:
            IRQ() = default;
            IRQ(APIC::APIC* apic, ISR::ISR* isr);
            bool Initialize(APIC::APIC* apic, ISR::ISR* isr);
            bool RegisterHandler(uint8_t irq, IRQHandler handler);
        private:
            static IRQ* instance;
            static void IRQDispatcher(ISR::Registers* regs);
            IRQHandler handlers[MAX_IRQ_HANDLERS]; // 96 total IRQ handlers can be mapped
            APIC::APIC* apic;
        };
    }
}