#pragma once
#include <stdint.h>

typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, kern_esp, ebx, edx, ecx, eax;
    uint32_t Interrupt, ErrCode;
    uint32_t eip, cs, eflags, esp, ss;
}__attribute__((packed)) Registers;

typedef void (*ISRHandler)(Registers* regs);

void ISR_Initialize(uint32_t ISRHandlersAddr);
void ISR_RegisterHandler(uint8_t interrupt, ISRHandler handler);