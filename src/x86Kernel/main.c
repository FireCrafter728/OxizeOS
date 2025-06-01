#include <Boot/bootparams.h>
#include <stdio.h>
#include <io.h>
#include <Interrupts/isr.h>

void __attribute__((section(".entry"))) _start(SystemInfo* System)
{
    clrscr();
    printf("Hello, world from x86Kern.exe!!!\r\n");

    CLI();

    ISR_Initialize(System->ISRHandlers);

    STI();

    HaltSystem();
}