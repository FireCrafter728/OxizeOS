#include <io.h>
#include <stdio.h>
#include <Boot/bootparams.h>
#include <Memory/memdefs.hpp>
#include <Interrupts/isr.hpp>
#include <Interrupts/irq.hpp>
#include <PIC/pic.hpp>
#include <string.h>

SystemInfo* System = nullptr;

x86Kernel::ISR::ISR* isr;
x86Kernel::PIC::PIC* pic;
x86Kernel::IRQ::IRQ* irq;

void __attribute__((section(".entry"))) InitializeInterrupts()
{
    static x86Kernel::ISR::ISR T_isr;
    static x86Kernel::PIC::PIC T_pic;
    static x86Kernel::IRQ::IRQ T_irq;

    isr = &T_isr;
    pic = &T_pic;
    irq = &T_irq;

    isr->Initialize(System->ISRHandlers);
    irq->Initialize(pic, isr);

    STI();
}

extern "C" void __attribute__((cdecl)) __attribute__((section(".entry"))) _x86kernel()
{
    clrscr();
    printf("Hello from x86Kern in C++!!!\r\n");

    System = (SystemInfo*)SYSTEM_PARAMETER_BLOCK_ADDR;

    // printf("ISRHanlders addr: 0x%x\r\n", System->ISRHandlers);

    InitializeInterrupts();

    // HaltSystem();
    while(1);
}