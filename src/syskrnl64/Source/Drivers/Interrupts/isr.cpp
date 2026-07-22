// SPDX-License-Identifier: GPL-3.0-or-later

#include <Drivers/Interrupts/isr.hpp>

using namespace SysKrnl64::ISR;

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

ASMCALL void ISR_Handler(Registers* regs)
{
    int intr = regs->interrupt;
    // Check if there is a handler present for the interrupt, and execute it if present
    if(ISR::handlers[intr]) {
        ISR::handlers[intr](regs);
        return;
    }

    // Handler not present, execute integrated handlers

    // integrated handler for exceptions
    if(intr < 32) {
        printf("[SYSKRNL64] [ISR] [CRITICAL]: CPU Exception occured(0x%X): %s\r\n", intr, ExceptionDescs[intr]);
        printf("[SYSKRNL64] [ISR] [CRITICAL]: CPU Exception Errcode: 0x%X, RIP: 0x%X\r\n", regs->errcode, regs->rip);
        HaltSystem();
    }

    // print a message about an unhandled interrupt

    printf("[SYSKRNL64] [ISR] [WARN]: Unhandled CPU Interrupt 0x%X\r\n", intr);
}