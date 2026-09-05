// SPDX-License-Identifier: GPL-3.0-or-later

#include <arch/x86_64/Interrupts/handlers.hpp>

#include <arch/x86_64/Utility/io.hpp>
#include <stdio.hpp>

#include <arch/x86_64/Interrupts/isr_mappings.hpp>

using namespace krnl;

bool IntHandlers::Initialize(ISR* isr, IRQ* irq)
{
	if(!isr || !irq) return false;

	isr->RegisterHandler(ISR_SVR, SpuriousInterruptHandler);

	// Register IRQ handlers

	// Register CPU Exception handlers

	isr->RegisterHandler(ISR_DOUBLE_FAULT, DoubleFaultHandler);
	isr->RegisterHandler(ISR_NON_MASKABLE_INTERRUPT_EXCEPTION, NonMaskableInterruptHandler);
	isr->RegisterHandler(ISR_MACHINE_CHECK_EXCEPTION, MachineCheckExceptionHandler);

	return true;
}

// ----------------- //
// APIC IRQ Handlers //
// ----------------- //

void IntHandlers::SpuriousInterruptHandler(ISR_InterruptStackFrame* regs)
{
	(void)regs;
	return;
}

// ---------------------- //
// CPU Exception handlers //
// ---------------------- //

void IntHandlers::DoubleFaultHandler(ISR_InterruptStackFrame* regs)
{
	printf("[SYSKRNL64] [CRITICAL]: DOUBLE FAULT EXCEPTION\r\n");
	printf("X86_64 REGISTER DUMP:\r\n");
	printf("DS=0x%llX, ES=0x%llX, RBP=0x%llX, KERNEL RSP=0x%llX\r\n", regs->ds, regs->es, regs->rbp, regs->kernelRsp);
	printf("RAX=0x%llX, RBX=0x%llX, RCX=0x%llX, RDX=0x%llX, RDI=0x%llX, RSI=0x%llX\r\n", regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rdi, regs->rsi);
	printf("R8=0x%llX, R9=0x%llX, R10=0x%llX, R11=0x%llX, R12=0x%llX, R13=0x%llX, R14=0x%llX, R15=0x%llX\r\n", regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13, regs->r14, regs->r15);
	printf("VECTOR: %llu, ERRCODE: 0x%llX\r\n", regs->interrupt, regs->errcode);
	printf("RIP=0x%llX, CS=0x%llX, RFLAGS=0x%llX\r\n", regs->rip, regs->cs, regs->rflags);

	// Check if ring transition happened between ring3 -> ring0
	if((regs->cs & 3) == 3)
	{
		printf("USER RSP=0x%llX, USER SS=0x%llX\r\n", regs->userRsp, regs->ss);
		printf("[INFO]: EXCEPTION OCCURRED IN RING 3\r\n");
	}
	else printf("[INFO]: EXCEPTION OCCURRED IN RING 0\r\n");
	HaltSystem();
}

void IntHandlers::NonMaskableInterruptHandler(ISR_InterruptStackFrame* regs)
{
	printf("[SYSKRNL64] [CRITICAL]: NON-MASKABLE INTERRUPT EXCEPTION\r\n");
	printf("X86_64 REGISTER DUMP:\r\n");
	printf("DS=0x%llX, ES=0x%llX, RBP=0x%llX, KERNEL RSP=0x%llX\r\n", regs->ds, regs->es, regs->rbp, regs->kernelRsp);
	printf("RAX=0x%llX, RBX=0x%llX, RCX=0x%llX, RDX=0x%llX, RDI=0x%llX, RSI=0x%llX\r\n", regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rdi, regs->rsi);
	printf("R8=0x%llX, R9=0x%llX, R10=0x%llX, R11=0x%llX, R12=0x%llX, R13=0x%llX, R14=0x%llX, R15=0x%llX\r\n", regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13, regs->r14, regs->r15);
	printf("VECTOR: %llu, ERRCODE: 0x%llX\r\n", regs->interrupt, regs->errcode);
	printf("RIP=0x%llX, CS=0x%llX, RFLAGS=0x%llX\r\n", regs->rip, regs->cs, regs->rflags);

	// Check if ring transition happened between ring3 -> ring0
	if((regs->cs & 3) == 3)
	{
		printf("USER RSP=0x%llX, USER SS=0x%llX\r\n", regs->userRsp, regs->ss);
		printf("[INFO]: EXCEPTION OCCURRED IN RING 3\r\n");
	}
	else printf("[INFO]: EXCEPTION OCCURRED IN RING 0\r\n");
	HaltSystem();
}

void IntHandlers::MachineCheckExceptionHandler(ISR_InterruptStackFrame* regs)
{
	printf("[SYSKRNL64] [CRITICAL]: MACHINE CHECK EXCEPTION\r\n");
	printf("X86_64 REGISTER DUMP:\r\n");
	printf("DS=0x%llX, ES=0x%llX, RBP=0x%llX, KERNEL RSP=0x%llX\r\n", regs->ds, regs->es, regs->rbp, regs->kernelRsp);
	printf("RAX=0x%llX, RBX=0x%llX, RCX=0x%llX, RDX=0x%llX, RDI=0x%llX, RSI=0x%llX\r\n", regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rdi, regs->rsi);
	printf("R8=0x%llX, R9=0x%llX, R10=0x%llX, R11=0x%llX, R12=0x%llX, R13=0x%llX, R14=0x%llX, R15=0x%llX\r\n", regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13, regs->r14, regs->r15);
	printf("VECTOR: %llu, ERRCODE: 0x%llX\r\n", regs->interrupt, regs->errcode);
	printf("RIP=0x%llX, CS=0x%llX, RFLAGS=0x%llX\r\n", regs->rip, regs->cs, regs->rflags);

	// Check if ring transition happened between ring3 -> ring0
	if((regs->cs & 3) == 3)
	{
		printf("USER RSP=0x%llX, USER SS=0x%llX\r\n", regs->userRsp, regs->ss);
		printf("[INFO]: EXCEPTION OCCURRED IN RING 3\r\n");
	}
	else printf("[INFO]: EXCEPTION OCCURRED IN RING 0\r\n");
	HaltSystem();
}