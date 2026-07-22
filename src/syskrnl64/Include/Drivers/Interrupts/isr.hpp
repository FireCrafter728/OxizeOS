// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |-----------------------------------------------------| //
// | OxizeOS Kernel Implementation                       | //
// | ISR: Driver for managing Interrupt Service Routines | //
// |-----------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

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
            uint64_t rsp, ss; // Only exists when there was a ring3->ring0 switch, to detect if that switch happened, perform (cs & 3) == 3
        };

        typedef void (*ISRHandler)(Registers* regs);

        enum ExceptionVector : uint8_t
        {
            DIVIDE_BY_ZERO_FAULT                    = 0,
            DEBUG_EXCEPTION                         = 1,
            NON_MASKABLE_INTERRUPT_EXCEPTION        = 2,
            BREAKPOINT_EXCEPTION                    = 3,
            OVERFLOW_EXCEPTION                      = 4,
            BOUND_RANGE_EXCEEDED_EXCEPTION          = 5,
            INVALID_OPCODE_EXCEPTION                = 6,
            DEVICE_NOT_AVAILABLE_EXCEPTION          = 7,
            DOUBLE_FAULT                            = 8,
            COPROCESSOR_SEGMENT_OVERRUN_EXCEPTION   = 9,
            INVALID_TSS_EXCEPTION                   = 10,
            SEGMENT_NOT_PRESENT_EXCEPTION           = 11,
            STACK_SEGMENT_FAULT                     = 12,
            GENERAL_PROTECTION_FAULT                = 13,
            PAGE_FAULT                              = 14,
        
            // 15 is reserved
        
            X87_FLOATING_POINT_EXCEPTION            = 16,
            ALIGNMENT_CHECK_EXCEPTION               = 17,
            MACHINE_CHECK_EXCEPTION                 = 18,
            SIMD_FLOATING_POINT_EXCEPTION           = 19,
            VIRTUALIZATION_EXCEPTION                = 20,
            CONTROL_PROTECTION_EXCEPTION            = 21,
        
            // 22-27 are reserved
        
            HYPERVISOR_INJECTION_EXCEPTION          = 28,
            VMM_COMMUNICATION_EXCEPTION             = 29,
            SECURITY_EXCEPTION                      = 30
        
            // 31 is reserved
        };

        class ISR
        {
        public:
            void Initialize();
            void RegisterHandler(int interrupt, ISRHandler handler);
            static ISRHandler handlers[256];
        };
    }
}