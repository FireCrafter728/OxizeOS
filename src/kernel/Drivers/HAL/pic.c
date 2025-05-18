#include <pic.h>
#include <io.h>

#define PIC1_COMMAND_PORT   0x20
#define PIC1_DATA_PORT      0x21
#define PIC2_COMMAND_PORT   0xA0
#define PIC2_DATA_PORT      0xA1

typedef enum {
    PIC_ICW1_ICW4           = 0x01,
    PIC_ICW1_SINGLE         = 0x02,
    PIC_ICW1_INTERVAL4      = 0x04,
    PIC_ICW1_LEVEL          = 0x08,
    PIC_ICW1_INITIALIZE     = 0x10,
} PIC_ICW1;

typedef enum {
    PIC_ICW4_8086           = 0x01,
    PIC_ICW4_AUTO_EOI       = 0x02,
    PIC_ICW4_BUFFER_MASTER  = 0x04,
    PIC_ICW4_BUFFER_SLAVE   = 0x00,
    PIC_ICW4_BUFFERED       = 0x08,
    PIC_ICW4_SFNM           = 0x10,

} PIC_ICW4;

typedef enum {
    PIC_OCW2_EOI            = 0x20,
    PIC_OCW2_SPECIFIC_EOI   = 0x40, 
} PIC_OCW2;

typedef enum {
    PIC_CMD_READ_IRR        = 0x0A,
    PIC_CMD_READ_ISR        = 0x0B,
} PIC_CMD;

void PIC_Configure(uint8_t offsetPic1, uint8_t offsetPic2)
{
    outb(PIC1_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    iowait();
    outb(PIC2_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    iowait();

    outb(PIC1_DATA_PORT, offsetPic1);
    iowait();
    outb(PIC2_DATA_PORT, offsetPic2);
    iowait();

    outb(PIC1_DATA_PORT, 0x4);
    iowait();
    outb(PIC2_DATA_PORT, 0x2);
    iowait();

    outb(PIC1_DATA_PORT, PIC_ICW4_8086);
    iowait();
    outb(PIC2_DATA_PORT, PIC_ICW4_8086);
    iowait();

    outb(PIC1_DATA_PORT, 0);
    iowait();
    outb(PIC2_DATA_PORT, 0);
    iowait();
}

void PIC_Mask(uint8_t irq)
{
    uint8_t port = PIC1_DATA_PORT;
    if(irq >= 8) {
        irq -= 8;
        port = PIC2_DATA_PORT;
    }
    uint8_t mask = inb(port);
    outb(port, mask | (1 << irq));
}

void PIC_Unmask(uint8_t irq)
{
    uint8_t port = PIC1_DATA_PORT;
    if(irq >= 8) {
        irq -= 8;
        port = PIC2_DATA_PORT;
    }
    uint8_t mask = inb(port);
    outb(port, mask & ~(1 << irq));
}

void PIC_Disable()
{
    outb(PIC1_DATA_PORT, 0xFF);
    iowait();
    outb(PIC2_DATA_PORT, 0xFF);
    iowait();
}

void PIC_SendEOI(uint8_t irq)
{
    uint8_t port = PIC1_COMMAND_PORT;
    if(irq >= 8) {
        irq -= 8;
        port = PIC2_COMMAND_PORT;
    }

    uint8_t L0 = 0, L1 = 0, L2 = 0;
    if(irq >= 4) {
        L0 = 1 << 0;
        irq -= 4;
    }
    if(irq >= 2) {
        L1 = 1 << 1;
        irq -= 2;
    }
    if(irq >= 1) L2 = 1 << 2;

    outb(port, L0 | L1 | L2 | PIC_OCW2_EOI | PIC_OCW2_SPECIFIC_EOI);
}

uint16_t PIC_GetIRR()
{
    outb(PIC1_COMMAND_PORT, PIC_CMD_READ_IRR);
    outb(PIC2_COMMAND_PORT, PIC_CMD_READ_IRR);
    return inb(PIC1_COMMAND_PORT) | (inb(PIC1_COMMAND_PORT) << 8);
}

uint16_t PIC_GetISR()
{
    outb(PIC1_COMMAND_PORT, PIC_CMD_READ_ISR);
    outb(PIC2_COMMAND_PORT, PIC_CMD_READ_ISR);
    return inb(PIC1_COMMAND_PORT) | (inb(PIC1_COMMAND_PORT) << 8);
}