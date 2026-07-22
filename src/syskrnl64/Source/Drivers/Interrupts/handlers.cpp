// SPDX-License-Identifier: GPL-3.0-or-later

#include <Drivers/Interrupts/handlers.hpp>

using namespace SysKrnl64::IntHandlers;

IntHandlers::IntHandlers(ISR::ISR* isr, IRQ::IRQ* irq)
{
    if(!Initialize(isr, irq)) HaltSystem();
}

bool IntHandlers::Initialize(ISR::ISR* isr, IRQ::IRQ* irq)
{
    if(!isr || !irq) return false;

    isr->RegisterHandler(ISR_SVR, SpuriousInterruptHandler);

    // Register IRQ handlers

    // Register CPU Exception handlers

    isr->RegisterHandler(ISR::DOUBLE_FAULT, DoubleFaultHandler);
    isr->RegisterHandler(ISR::NON_MASKABLE_INTERRUPT_EXCEPTION, NonMaskableInterruptHandler);
    isr->RegisterHandler(ISR::MACHINE_CHECK_EXCEPTION, MachineCheckExceptionHandler);

    return true;
}

// ----------------- //
// APIC IRQ Handlers //
// ----------------- //

void IntHandlers::SpuriousInterruptHandler(ISR::Registers* regs)
{
    (void)regs;
    return;
}

// ---------------------- //
// CPU Exception handlers //
// ---------------------- //

void IntHandlers::DoubleFaultHandler(ISR::Registers* regs)
{
    printf("[SYSKRNL64] [CRITICAL]: DOUBLE FAULT EXCEPTION\r\n");
    printf("X86_64 REGISTER DUMP:\r\n");
    printf("DS=0x%llX, ES=0x%llX, RBP=0x%llX, KERNEL RSP=0x%llX\r\n", regs->ds, regs->es, regs->rbp, regs->kern_rsp);
    printf("RAX=0x%llX, RBX=0x%llX, RCX=0x%llX, RDX=0x%llX, RDI=0x%llX, RSI=0x%llX\r\n", regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rdi, regs->rsi);
    printf("R8=0x%llX, R9=0x%llX, R10=0x%llX, R11=0x%llX, R12=0x%llX, R13=0x%llX, R14=0x%llX, R15=0x%llX\r\n", regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13, regs->r14, regs->r15);
    printf("VECTOR: %d, ERRCODE: 0x%llX\r\n", regs->interrupt, regs->errcode);
    printf("RIP=0x%llX, CS=0x%llX, RFLAGS=0x%llX\r\n", regs->rip, regs->cs, regs->rflags);

    // Check if ring transition happened between ring3 -> ring0
    if((regs->cs & 3) == 3)
    {
        printf("USER RSP=0x%llX, USER SS=0x%llX\r\n", regs->rsp, regs->ss);
        printf("[INFO]: EXCEPTION OCCURRED IN RING 3");
    }
    else printf("[INFO]: EXCEPTION OCCURRED IN RING 0");
    HaltSystem();
}

void IntHandlers::NonMaskableInterruptHandler(ISR::Registers* regs)
{
    printf("[SYSKRNL64] [CRITICAL]: NON-MASKABLE INTERRUPT EXCEPTION\r\n");
    printf("X86_64 REGISTER DUMP:\r\n");
    printf("DS=0x%llX, ES=0x%llX, RBP=0x%llX, KERNEL RSP=0x%llX\r\n", regs->ds, regs->es, regs->rbp, regs->kern_rsp);
    printf("RAX=0x%llX, RBX=0x%llX, RCX=0x%llX, RDX=0x%llX, RDI=0x%llX, RSI=0x%llX\r\n", regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rdi, regs->rsi);
    printf("R8=0x%llX, R9=0x%llX, R10=0x%llX, R11=0x%llX, R12=0x%llX, R13=0x%llX, R14=0x%llX, R15=0x%llX\r\n", regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13, regs->r14, regs->r15);
    printf("VECTOR: %d, ERRCODE: 0x%llX\r\n", regs->interrupt, regs->errcode);
    printf("RIP=0x%llX, CS=0x%llX, RFLAGS=0x%llX\r\n", regs->rip, regs->cs, regs->rflags);

    // Check if ring transition happened between ring3 -> ring0
    if((regs->cs & 3) == 3)
    {
        printf("USER RSP=0x%llX, USER SS=0x%llX\r\n", regs->rsp, regs->ss);
        printf("[INFO]: EXCEPTION OCCURRED IN RING 3");
    }
    else printf("[INFO]: EXCEPTION OCCURRED IN RING 0");
    HaltSystem();
}

void IntHandlers::MachineCheckExceptionHandler(ISR::Registers* regs)
{
    printf("[SYSKRNL64] [CRITICAL]: MACHINE CHECK EXCEPTION\r\n");
    printf("X86_64 REGISTER DUMP:\r\n");
    printf("DS=0x%llX, ES=0x%llX, RBP=0x%llX, KERNEL RSP=0x%llX\r\n", regs->ds, regs->es, regs->rbp, regs->kern_rsp);
    printf("RAX=0x%llX, RBX=0x%llX, RCX=0x%llX, RDX=0x%llX, RDI=0x%llX, RSI=0x%llX\r\n", regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rdi, regs->rsi);
    printf("R8=0x%llX, R9=0x%llX, R10=0x%llX, R11=0x%llX, R12=0x%llX, R13=0x%llX, R14=0x%llX, R15=0x%llX\r\n", regs->r8, regs->r9, regs->r10, regs->r11, regs->r12, regs->r13, regs->r14, regs->r15);
    printf("VECTOR: %d, ERRCODE: 0x%llX\r\n", regs->interrupt, regs->errcode);
    printf("RIP=0x%llX, CS=0x%llX, RFLAGS=0x%llX\r\n", regs->rip, regs->cs, regs->rflags);

    // Check if ring transition happened between ring3 -> ring0
    if((regs->cs & 3) == 3)
    {
        printf("USER RSP=0x%llX, USER SS=0x%llX\r\n", regs->rsp, regs->ss);
        printf("[INFO]: EXCEPTION OCCURRED IN RING 3");
    }
    else printf("[INFO]: EXCEPTION OCCURRED IN RING 0");
    HaltSystem();
}