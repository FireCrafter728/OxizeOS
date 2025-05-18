#include <Boot/bootparams.h>
#include <stdio.h>

void __attribute__((section(".entry"))) _start(SystemInfo* System)
{
    printf("Hello, world from x86Kern.exe!!!\r\n");
    while(1);
}