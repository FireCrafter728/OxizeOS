#pragma once
#include <PIC/pic.hpp>
#include <Interrupts/isr.hpp>

namespace x86Kernel
{
    namespace IRQ
    {
        typedef void (*IRQHandler)(ISR::Registers* regs);
        class IRQ
        {
        public:
            IRQ() = default;
            IRQ(PIC::PIC* pic, ISR::ISR* isr);
            void Initialize(PIC::PIC* pic, ISR::ISR* isr);
            void RegisterHandler(uint8_t irq, IRQHandler handler);
        private:
            PIC::PIC* pic;
            IRQHandler handlers[16];
            static void IRQCaller(ISR::Registers* regs);
        };
    }
}