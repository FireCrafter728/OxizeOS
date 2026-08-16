// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>
#include <arch/x86_64/Interrupts/irq.hpp>

namespace krnl
{
	class IntHandlers
	{
	public:
		bool Initialize(ISR* isr, IRQ* irq);
	private:
		// APIC IRQs
		static void SpuriousInterruptHandler(ISR_InterruptStackFrame* regs);

		// CPU Exceptions

		static void DoubleFaultHandler(ISR_InterruptStackFrame* regs);
		static void NonMaskableInterruptHandler(ISR_InterruptStackFrame* regs);
		static void MachineCheckExceptionHandler(ISR_InterruptStackFrame* regs); 
	};
}