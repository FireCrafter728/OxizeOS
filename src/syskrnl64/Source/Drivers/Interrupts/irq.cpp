// SPDX-License-Identifier: GPL-3.0-or-later

#include <Drivers/Interrupts/irq.hpp>

using namespace SysKrnl64::IRQ;

IRQ* IRQ::instance;

IRQ::IRQ(APIC::APIC* apic, ISR::ISR* isr)
{
    if(!Initialize(apic, isr)) HaltSystem();
}

bool IRQ::Initialize(APIC::APIC* apic, ISR::ISR* isr)
{
    if(!apic || !isr) return false;

    this->apic = apic;

    instance = this;

    for(uint8_t i = 0; i < MAX_IRQ_HANDLERS; i++) {
        isr->RegisterHandler(IRQ_BASE + i, IRQDispatcher);
        handlers[i] = nullptr;
    }

    return true;
}

void IRQ::IRQDispatcher(ISR::Registers* regs)
{
    IRQHandler handler = instance->handlers[regs->interrupt - IRQ_BASE];
    if(handler) handler(regs);
    else printf("[SYSKRNL64] [IRQ] [WARN]: Unhandled IRQ 0x%X\r\n", regs->interrupt);

    instance->apic->SendEOI();
}

bool IRQ::RegisterHandler(uint8_t irq, IRQHandler handler)
{
    if(irq >= IRQ_COUNT) return false;
    handlers[irq] = handler;
    return true;
}