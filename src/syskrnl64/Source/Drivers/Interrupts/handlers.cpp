#include <Drivers/Interrupts/handlers.hpp>

using namespace SysKrnl64::IntHandlers;

size_t SysKrnl64::IntHandlers::ticks = 0;

IntHandlers::IntHandlers(ISR::ISR* isr, IRQ::IRQ* irq)
{
    if(!Initialize(isr, irq)) HaltSystem();
}

bool IntHandlers::Initialize(ISR::ISR* isr, IRQ::IRQ* irq)
{
    if(!isr || !irq) return false;

    isr->RegisterHandler(ISR_SVR, SpuriousInterruptHandler);

    irq->RegisterHandler(IRQ_PIT, PITHandler);
    irq->RegisterHandler(IRQ_KBD, KeyboardHandler);

    return true;
}

void IntHandlers::SpuriousInterruptHandler(ISR::Registers* regs)
{
    (void)regs;
    return;
}

void IntHandlers::PITHandler(ISR::Registers* regs)
{
    ticks++;
}

void IntHandlers::KeyboardHandler(ISR::Registers* regs)
{   
    
}