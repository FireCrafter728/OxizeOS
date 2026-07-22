// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |---------------------------------------------------------| //
// | OxizeOS Kernel Implementation                           | //
// | IDT: Driver for managing the Interrupt Descriptor Table | //
// |---------------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdint.hpp>

#ifndef PACK
#define PACK __attribute__((packed))
#endif

#ifndef FPACK
#define FPACK __attribute__((packed, aligned(1)))
#endif

#ifndef ASMCALL
#define ASMCALL extern "C"
#endif

#define IDT_FLAG_GATE_TASK          SysKrnl64::IDT::FLAG_GATE_TASK
#define IDT_FLAG_GATE_16BIT_INT     SysKrnl64::IDT::FLAG_GATE_16BIT_INT
#define IDT_FLAG_GATE_16BIT_TRAP    SysKrnl64::IDT::FLAG_GATE_16BIT_TRAP
#define IDT_FLAG_GATE_32BIT_INT     SysKrnl64::IDT::FLAG_GATE_32BIT_INT
#define IDT_FLAG_GATE_32BIT_TRAP    SysKrnl64::IDT::FLAG_GATE_32BIT_TRAP
#define IDT_FLAG_GATE_64BIT_INT     SysKrnl64::IDT::FLAG_GATE_64BIT_INT
#define IDT_FLAG_GATE_64BIT_TRAP    SysKrnl64::IDT::FLAG_GATE_64BIT_TRAP
#define IDT_FLAG_RING0              SysKrnl64::IDT::FLAG_RING0 
#define IDT_FLAG_RING1              SysKrnl64::IDT::FLAG_RING1
#define IDT_FLAG_RING2              SysKrnl64::IDT::FLAG_RING2
#define IDT_FLAG_RING3              SysKrnl64::IDT::FLAG_RING3
#define IDT_FLAG_PRESENT            SysKrnl64::IDT::FLAG_PRESENT

namespace SysKrnl64
{
    namespace IDT
    {
        struct PACK IDT_Entry
        {
            uint16_t BaseLow;
            uint16_t SegmentSelector;
            uint8_t IST;
            uint8_t Flags;
            uint16_t BaseMiddle;
            uint32_t BaseHigh;
            uint32_t Reserved;
        };

        struct FPACK IDT_Desc
        {
            uint16_t Limit;
            IDT_Entry* entries;
        };

        enum IDT_FLAGS
        {
            FLAG_GATE_TASK = 0x05,
            FLAG_GATE_16BIT_INT = 0x06,
            FLAG_GATE_16BIT_TRAP = 0x07,
            
            FLAG_GATE_32BIT_INT = 0x0E,
            FLAG_GATE_32BIT_TRAP = 0x0F,

            FLAG_GATE_64BIT_INT = 0x0E,
            FLAG_GATE_64BIT_TRAP = 0x0F,

            FLAG_RING0 = 0x00,
            FLAG_RING1 = 0x20,
            FLAG_RING2 = 0x40,
            FLAG_RING3 = 0x60,

            FLAG_PRESENT = 0x80,
        };

        class IDT
        {
        public:
            void Initialize();
            static void SetGate(int interrupt, void* base, uint16_t segmentDescriptor, uint8_t flags, uint8_t interruptIST = 0);
            static void EnableGate(int interrupt);
            static void DisableGate(int interrupt);
        private:
            IDT_Desc idtDesc;
            static IDT_Entry entries[256];
        };
    }
}
