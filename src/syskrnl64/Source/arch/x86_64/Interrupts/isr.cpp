// SPDX-License-Identifier: GPL-3.0-or-later

#include <arch/x86_64/Interrupts/isr.hpp>
#include <arch/x86_64/Interrupts/idt.hpp>
#include <arch/x86_64/Utility/io.hpp>
#include <arch/x86_64/MP/lpdata.hpp>

#include <stdio.hpp>


using namespace krnl;

ISRHandler ISR::handlers[256];

void ISR_InitializeGates();

void ISR::Initialize()
{
	ISR_InitializeGates();
	for(int i = 0; i < 256; i++)
		IDT::IDT::EnableGate(i);
}

void ISR::RegisterHandler(int interrupt, ISRHandler handler)
{
	if(handler) handlers[interrupt] = handler;
	else {
		printf("[SYSKRNL64] [ISR] [ERROR]: Tried to register a interrupt handler which's address was NULL\r\n");
		HaltSystem();
	}
}

const char* ExceptionDescs[] = {
	"Divide Error (#DE)",
	"Debug (#DB)",
	"Non-Maskable Interrupt",
	"Breakpoint (#BP)",
	"Overflow (#OF)",
	"BOUND Range Exceeded (#BR)",
	"Invalid Opcode (#UD)",
	"Device Not Available (#NM)",
	"Double Fault (#DF)",
	"Coprocessor segment overrun",
	"Invalid TSS (#TS)",
	"Segment Not Present (#NP)",
	"Stack Fault (#SS)",
	"General Protection Fault (#GP)",
	"Page Fault (#PF)",
	"Intel Reserved",
	"x87 FPU Floating-Point Error (#MF)",
	"Alignment Check fault (#AC)",
	"Machine-Check fault (#MC)",
	"SIMD Floating Point fault (#XM)",
	"Virtualization Exception (#VE)",
	"Control Protection fault (#CP)",
	"Intel Reserved",
	"Intel Reserved",
	"Intel Reserved",
	"Intel Reserved",
	"Intel Reserved",
	"Intel Reserved",
	"Hypervisor Injection Fault (#HV)",
	"VMM Communication Fault (#VC)",
	"Security Fault (#SX)",
	"Intel Reserved"
};

ASMCALL void ISR_Handler(ISR_InterruptStackFrame* regs)
{
	int intr = regs->interrupt;
	// Check if there is a handler present for the interrupt, and execute it if present
	if(ISR::handlers[intr]) {
		ISR::handlers[intr](regs);
		return;
	}

	// Handler not present, execute integrated handlers

	// Get the LP Index
	LPSpecificData* lpSpecificData = LPData::GetLPDataForCurrentLP();
	LPID lpId = lpSpecificData->identity.lpid;

	// integrated handler for exceptions
	if(intr < 32) {
		printf("[SYSKRNL64] [ISR] [CRITICAL]: LP %lu Exception occured(0x%X): %s\r\n", lpId, intr, ExceptionDescs[intr]);
		printf("[SYSKRNL64] [ISR] [CRITICAL]: LP %lu Exception Errcode: 0x%llX, RIP: 0x%llX\r\n", lpId, regs->errcode, regs->rip);
		HaltSystem();
	}

	// print a message about an unhandled interrupt

	printf("[SYSKRNL64] [ISR] [WARN]: Unhandled LP %lu Interrupt 0x%X\r\n", lpId, intr);
}