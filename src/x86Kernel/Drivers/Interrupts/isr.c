#include <Interrupts/isr.h>
#include <stddef.h>

ISRHandler* ISRHandlers = NULL;

void ISR_Initialize(uint32_t ISRHandlersAddr)
{
    ISRHandlers = (ISRHandler*)(uintptr_t)ISRHandlersAddr;
}

void ISR_RegisterHandler(uint8_t interrupt, ISRHandler handler)
{
    ISRHandlers[interrupt] = handler;
}