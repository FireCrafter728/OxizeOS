#include <Interrupts/irq.hpp>
#include <stdio.h>

using namespace x86Kernel::IRQ;

static IRQ* instance;

void IRQ::IRQCaller(ISR::Registers* regs)
{
    if(instance->handlers[regs->Interrupt - PIC1_REMAP_OFFSET] != nullptr) instance->handlers[regs->Interrupt - PIC1_REMAP_OFFSET](regs);
    else printf("[PIC IRQ] [WARNING]: Unhandled IRQ %u\r\n", regs->Interrupt - PIC1_REMAP_OFFSET);
    instance->pic->SendEOI(regs->Interrupt - PIC1_REMAP_OFFSET);
}

IRQ::IRQ(PIC::PIC* pic, ISR::ISR* isr)
{
    Initialize(pic, isr);
}

void IRQ::Initialize(PIC::PIC* pic, ISR::ISR* isr)
{
    this->pic = pic;
    instance = this;
    for(int i = 0; i < PIC_CASCADE_MAX_IRQS; i++) isr->RegisterHandler(i + PIC1_REMAP_OFFSET, IRQCaller);
}

void IRQ::RegisterHandler(uint8_t irq, IRQHandler handler)
{
    this->handlers[irq] = handler;
}