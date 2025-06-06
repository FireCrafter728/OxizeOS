#include <PIC/pic.hpp>
#include <io.h>

#define PIC1_COMMAND_PORT 0x20
#define PIC1_DATA_PORT 0x21
#define PIC2_COMMAND_PORT 0xA0
#define PIC2_DATA_PORT 0xA1

using namespace x86Kernel::PIC;

PIC::PIC()
{
    // ICW 1, ICW4 needed, Initialize
    outb(PIC1_COMMAND_PORT, ICW1_ICW4 | ICW1_INITIALIZE);
    iowait();
    outb(PIC2_COMMAND_PORT, ICW1_ICW4 | ICW1_INITIALIZE);
    iowait();

    // ICW 2, IRQ Addresses to be mapped as ISRs
    outb(PIC1_DATA_PORT, PIC1_REMAP_OFFSET);
    iowait();
    outb(PIC2_DATA_PORT, PIC2_REMAP_OFFSET);
    iowait();

    // ICW 3, tell PIC1 that it has a slave at IRQ 2
    // ICW 3, tell PIC2 it's cascade identity
    outb(PIC1_DATA_PORT, 0x4);
    iowait();
    outb(PIC2_DATA_PORT, 0x2);
    iowait();

    // ICW4, Tell it's in the 8086 / 8088 mode of operations
    outb(PIC1_DATA_PORT, ICW4_8086_8088);
    iowait();
    outb(PIC2_DATA_PORT, ICW4_8086_8088);
    iowait();

    // Clear PIC data ports
    outb(PIC1_DATA_PORT, 0);
    iowait();
    outb(PIC2_DATA_PORT, 0);
    iowait();
}

void PIC::Mask(uint8_t irq)
{
    uint8_t port = PIC1_DATA_PORT;
    if (irq > 8)
    {
        port = PIC1_DATA_PORT;
        irq -= 8;
    }
    uint8_t mask = inb(port);
    outb(port, mask | (1 << irq));
}

void PIC::Unmask(uint8_t irq)
{
    uint8_t port = PIC1_DATA_PORT;
    if (irq > 8)
    {
        port = PIC1_DATA_PORT;
        irq -= 8;
    }
    uint8_t mask = inb(port);
    outb(port, mask & ~(1 << irq));
}

void PIC::SendEOI(uint8_t irq)
{
    if (irq >= 8)
    {
        outb(PIC2_COMMAND_PORT, CMD_SPECIFIC_EOI | (irq - 8));
        iowait();
        outb(PIC1_COMMAND_PORT, CMD_SPECIFIC_EOI | 2);
        iowait();
    }
    else
    {
        outb(PIC1_COMMAND_PORT, CMD_SPECIFIC_EOI | irq);
        iowait();
    }
}

void PIC::Disable()
{
    outb(PIC1_DATA_PORT, 0xFF);
    iowait();
    outb(PIC2_DATA_PORT, 0xFF);
    iowait();
}

uint8_t PIC::ReadIRR()
{
    outb(PIC1_COMMAND_PORT, CMD_READ_IRR);
    iowait();
    outb(PIC2_COMMAND_PORT, CMD_READ_IRR);
    iowait();
    return inb(PIC2_COMMAND_PORT) | (inb(PIC2_COMMAND_PORT) << 8);
}

uint8_t PIC::ReadISR()
{
    outb(PIC1_COMMAND_PORT, CMD_READ_ISR);
    iowait();
    outb(PIC2_COMMAND_PORT, CMD_READ_ISR);
    iowait();
    return inb(PIC2_COMMAND_PORT) | (inb(PIC2_COMMAND_PORT) << 8);
}