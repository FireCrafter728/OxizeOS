#pragma once
#include <stdint.h>

#define PIC1_REMAP_OFFSET           32
#define PIC2_REMAP_OFFSET           40

#define PIC_CASCADE_MAX_IRQS        16
#define PIC_SINGLE_MAX_IRQS         8

namespace x86Kernel
{
    namespace PIC
    {
        enum ICW1 {
            ICW1_ICW4               = 0x01,
            ICW1_SINGLE             = 0x02,
            ICW1_INTERVAL4          = 0x04,
            ICW1_LEVEL              = 0x08,
            ICW1_INITIALIZE         = 0x10,
        };

        enum ICW4 {
            ICW4_8086_8088          = 0x01,
            ICW4_AUTO_EOI           = 0x02,
            ICW4_BUFFER_MASTER      = 0x04,
            ICW4_BUFFER_SLAVE       = 0x00,
            ICW4_BUFFERED           = 0x08,
            ICW4_SFNM               = 0x10,
        };

        enum CMD {
            CMD_EOI                 = 0x20,
            CMD_SPECIFIC_EOI        = 0x60,
            CMD_READ_IRR            = 0x0A,
            CMD_READ_ISR            = 0x0B,
        };

        class PIC
        {
        public:
            PIC();
            void Mask(uint8_t irq);
            void Unmask(uint8_t irq);
            void SendEOI(uint8_t irq);
            void Disable();
            uint8_t ReadIRR();
            uint8_t ReadISR();
        };
    }
}