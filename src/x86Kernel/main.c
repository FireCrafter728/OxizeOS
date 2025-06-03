#include <Boot/bootparams.h>
#include <stdio.h>
#include <io.h>
#include <Interrupts/isr.h>
#include <test.h>
#include <Memory/memdefs.h>

void __attribute__((cdecl)) __attribute__((section(".entry"))) _x86kernel()
{
    clrscr();
    printf("Hello, world from x86Kern.exe!!!\r\n");

    SystemInfo* System = (SystemInfo*)SYSTEM_PARAMETER_BLOCK_ADDR;

    ISR_Initialize(System->ISRHandlers);

    STI();

    CPPTest();

    HaltSystem();
    while(1);
}