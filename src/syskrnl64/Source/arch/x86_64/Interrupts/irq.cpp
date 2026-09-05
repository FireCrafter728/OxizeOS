// SPDX-License-Identifier: GPL-3.0-or-later

#include <arch/x86_64/Interrupts/irq.hpp>

#include <arch/x86_64/Utility/io.hpp>
#include <stdio.hpp>

#include <arch/x86_64/Interrupts/isr_mappings.hpp>

using namespace krnl;

IRQ* IRQ::instance;

bool IRQ::Initialize(APIC* apic, ISR* isr)
{
	if(!apic || !isr) return false;

	this->apic = apic;

	instance = this;

	for(uint8_t i = 0; i < IRQ_MAX_HANDLERS; i++) {
		isr->RegisterHandler(IRQ_BASE + i, IRQDispatcher);
		handlers[i] = nullptr;
	}

	return true;
}

void IRQ::IRQDispatcher(ISR_InterruptStackFrame* regs)
{
	IRQHandler handler = instance->handlers[regs->interrupt - IRQ_BASE];
	if(handler) handler(regs);
	else printf("[SYSKRNL64] [IRQ] [WARN]: Unhandled IRQ 0x%llX\r\n", regs->interrupt);

	instance->apic->SendEOI();
}

bool IRQ::RegisterHandler(uint8_t irq, IRQHandler handler)
{
	if(irq >= IRQ_COUNT) return false;
	handlers[irq] = handler;
	return true;
}