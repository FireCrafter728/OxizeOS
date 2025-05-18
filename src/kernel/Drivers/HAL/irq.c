#include <irq.h>
#include <pic.h>
#include <ISRMappings.h>
#include <io.h>
#include <stddef.h>
#include <stdio.h>

IRQHandler IRQHandlers[16];

void IRQ_Handler(Registers* regs)
{
    int irq = regs->Interrupt - PIC1_REMAP_OFFSET;
    if(IRQHandlers[irq] == NULL) printf("Unhandled IRQ %d\r\n", irq);
    else IRQHandlers[irq](regs);

    PIC_SendEOI(irq);
}

void IRQ_Initialize()
{
    PIC_Configure(PIC1_REMAP_OFFSET, PIC2_REMAP_OFFSET);
    for(uint8_t i = 0; i < 16; i++) {
        ISR_RegisterHandler(PIC1_REMAP_OFFSET + i, IRQ_Handler);
        PIC_Mask(i);
    }
    STI();
}

void IRQ_RegisterHandler(int irq, IRQHandler handler)
{
    IRQHandlers[irq] = handler;
}