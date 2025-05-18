#include <isr.h>
#include <idt.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <io.h>

static const char* const Exceptions[] = {
    "Divide by zero error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception ",
    "",
    "",
    "",
    "",
    "",
    "",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    ""
};

uint8_t INT_ERRCODES[] = {
    8, 10, 11, 12, 13, 14, 17, 21, 29, 30
};

ISRHandler ISRHandlers[256];

void ISR_InitializeGates();

void __attribute__((cdecl)) ISR_Handler(Registers* regs)
{
    if (ISRHandlers[regs->Interrupt] != NULL) {
        ISRHandlers[regs->Interrupt](regs);
    } else {
        if (regs->Interrupt < 32) {
            bool ErrCode = false;
            for (int i = 0; i < sizeof(INT_ERRCODES) / sizeof(INT_ERRCODES[0]); i++) {
                if (regs->Interrupt == INT_ERRCODES[i]) {
                    ErrCode = true;
                    break;
                }
            }
            printf("Unhandled exception %u: %s", regs->Interrupt, Exceptions[regs->Interrupt]);
            if (ErrCode)
                printf(", error code: %u\r\n", regs->ErrCode);
            else
                printf("\r\n");
        } else {
            printf("Unhandled interrupt %u\r\n", regs->Interrupt);
        }

        printf("DS: 0x%x\r\n", regs->ds);
        printf("EDI: 0x%x, ESI: 0x%x, EBP: 0x%x, KERN_ESP: 0x%x\r\n", 
               regs->edi, regs->esi, regs->ebp, regs->kern_esp);
        printf("EBX: 0x%x, EDX: 0x%x, ECX: 0x%x, EAX: 0x%x\r\n", 
               regs->ebx, regs->edx, regs->ecx, regs->eax);
        printf("EIP: 0x%x, CS: 0x%x, EFLAGS: 0x%x\r\n", 
               regs->eip, regs->cs, regs->eflags);
        printf("ESP: 0x%x, SS: 0x%x\r\n", regs->esp, regs->ss);

        HaltSystem();
    }
}

void ISR_RegisterHandler(uint8_t interrupt, ISRHandler handler)
{
    ISRHandlers[interrupt] = handler;
}

void ISR_Initialize()
{
    ISR_InitializeGates();
    for(int i = 0; i < 256; i++) IDT_EnableGate(i);
}

