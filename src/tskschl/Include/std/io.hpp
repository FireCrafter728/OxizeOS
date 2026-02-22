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