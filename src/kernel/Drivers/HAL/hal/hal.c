#include <hal/hal.h>

#include <gdt.h>
#include <idt.h>
#include <isr.h>

void HAL_Initialize()
{
    GDT_Initialize();
    IDT_Initialize();
    ISR_Initialize();
}