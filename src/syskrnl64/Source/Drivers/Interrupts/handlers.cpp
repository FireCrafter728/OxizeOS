#include <Drivers/Interrupts/handlers.hpp>

using namespace SysKrnl64::IntHandlers;

static const char scancodeMap[128] =
{
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/',
    0,  '*', 0,  ' ', 0
};

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

    // Setup keyboard handler
    outb(0x64, 0xAD);
    outb(0x64, 0xA7);
    while(inb(0x64) & 1) (void)inb(0x60);

    outb(0x64, 0x20);
    uint8_t cfg = inb(0x60);

    cfg |= 0x01;
    cfg &= ~0x10;
    outb(0x64, 0x60);
    outb(0x60, cfg);

    outb(0x64, 0xAA);
    uint8_t response = inb(0x60);
    if(response == 0xFC) {
        printf("[SYSKRNL64] [INT-HANDLERS] [ERROR]: Failed to initialize PS/2 keyboard controller\r\n");
        return false;
    }
    if(response == 0x55) printf("[SYSKRNL64] [INT-HANDLERS] [INFO]: Sucessfully initialized PS/2 keyboard controller\r\n");

    outb(0x64, 0xAE);
    outb(0x60, 0xF4);

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
    uint8_t scancode = inb(0x60);

    bool released = scancode & 0x80;
    uint8_t key = scancode & 0x7F;

    if(released) return;

    char c = scancodeMap[key];

    printf("Key pressed: %c\r\n", c);
}