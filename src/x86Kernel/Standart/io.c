#include <stdio.h>
#include <io.h>

#define UNUSED_PORT 0x80

void __attribute__((cdecl)) Halt();

void HaltSystem()
{
    printf("System halted\r\n");
    CLI();
    Halt();
}

void iowait()
{
    outb(UNUSED_PORT, 0);
}