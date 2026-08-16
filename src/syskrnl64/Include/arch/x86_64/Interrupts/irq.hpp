// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>
#include <arch/x86_64/ACPI/apic.hpp>
#include <arch/x86_64/Interrupts/isr.hpp>

namespace krnl
{
	constexpr uint8_t IRQ_MAX_HANDLERS = 96;

	typedef void (*IRQHandler)(ISR_InterruptStackFrame* regs);

	class IRQ
	{
	public:
		bool Initialize(APIC* apic, ISR* isr);
		bool RegisterHandler(uint8_t irq, IRQHandler handler);
	private:
		static IRQ* instance;
		static void IRQDispatcher(ISR_InterruptStackFrame* regs);
		IRQHandler handlers[IRQ_MAX_HANDLERS]; // 96 total IRQ handlers can be mapped
		APIC* apic;
	};
}