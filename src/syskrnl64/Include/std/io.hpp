#pragma once

#ifndef ASMCALL
#define ASMCALL extern "C"
#endif

void HaltSystem();
ASMCALL void HaltSystemImpl();

ASMCALL void InvalidatePage(uintptr_t virt);

ASMCALL void outb(uint16_t port, uint8_t value);
ASMCALL uint8_t inb(uint16_t port);

ASMCALL void TestInt();

ASMCALL void DisableInterrupts();
ASMCALL void EnableInterrupts();

ASMCALL void GetCPUID(uint32_t leaf, uint32_t subLeaf, void* bufferOut);

ASMCALL uint64_t RDMSR(uint32_t msr);
ASMCALL void WRMSR(uint32_t msr, uint64_t value);