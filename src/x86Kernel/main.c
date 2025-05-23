#include <Boot/bootparams.h>
#include <stdio.h>
#include <io.h>

void __attribute__((section(".entry"))) _start(SystemInfo* System)
{
    clrscr();
    printf("Hello, world from x86Kern.exe!!!\r\n");
    HaltSystem();
}