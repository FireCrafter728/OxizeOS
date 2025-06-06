#include <Interrupts/isr.hpp>
#include <stddef.h>
#include <stdio.h>

using namespace x86Kernel::ISR;

ISR::ISR(uint32_t ISRHandlersAddr)
{
    Initialize(ISRHandlersAddr);
}

void ISR::Initialize(uint32_t ISRHandlersAddr)
{
    ISRHandlers = (ISRHandler*)(uintptr_t)ISRHandlersAddr;
}

void ISR::RegisterHandler(uint8_t interrupt, ISRHandler handler)
{
    ISRHandlers[interrupt] = handler;
}