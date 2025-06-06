#pragma once
#include <stdint.h>

namespace x86Kernel
{
    namespace ISR
    {
        struct __attribute__((packed)) Registers
        {
            uint32_t ds;
            uint32_t edi, esi, ebp, kern_esp, ebx, edx, ecx, eax;
            uint32_t Interrupt, ErrCode;
            uint32_t eip, cs, eflags, esp, ss;
        };
        
        typedef void (*ISRHandler)(Registers *regs);

        class ISR
        {
        public:
            ISR() = default;
            ISR(uint32_t ISRHandlersAddr);
            void Initialize(uint32_t ISRHandlersAddr);
            void RegisterHandler(uint8_t interrupt, ISRHandler handler);
            ~ISR() = default;
        private:
            ISRHandler* ISRHandlers = nullptr;
        };
    }
}