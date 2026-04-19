#pragma once

#include <stdint.hpp>

#ifndef PACK
#define PACK __attribute__((packed))
#endif

#ifndef ASMCALL
#define ASMCALL extern "C"
#endif

namespace SysKrnl64
{
    namespace ISR
    {
        struct PACK Registers
        {
            uint64_t ds, es, rbp, kern_rsp;
            uint64_t rax, rbx, rcx, rdx, rdi, rsi;
            uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
            uint64_t interrupt, errcode;
            uint64_t rip, cs, rflags;
            uint64_t rsp, ss;
        };

        typedef void (*ISRHandler)(Registers* regs);

        class ISR
        {
        public:
            void Initialize();
            void RegisterHandler(int interrupt, ISRHandler handler);
            static ISRHandler handlers[256];
        };
    }
}