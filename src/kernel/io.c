#include <stdio.h>
#include <io.h>


void __attribute__((cdecl)) Halt();

void HaltSystem()
{
    printf("System halted\r\n");
    CLI();
    Halt();
}

#define UNUSED_PORT 0x80

void iowait()
{
    outb(UNUSED_PORT, 0);
}