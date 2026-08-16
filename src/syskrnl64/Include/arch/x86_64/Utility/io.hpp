// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// x86_64 specific assembly functions

#include <main/defs.hpp>
#include <arch/x86_64/std/stdint.hpp>
#include <arch/x86_64/std/stddef.hpp>

void HaltSystem();
ASMCALL void HaltSystemImpl();

ASMCALL void Pause();

ASMCALL void InvalidatePage(uintptr_t virt);

ASMCALL void outb(uint16_t port, uint8_t value);
ASMCALL uint8_t inb(uint16_t port);

ASMCALL void TestInt();

ASMCALL void DisableInterrupts();
ASMCALL void EnableInterrupts();

ASMCALL void GetCPUID(uint32_t leaf, uint32_t subLeaf, void* bufferOut);

ASMCALL uint64_t RDMSR(uint32_t msr);
ASMCALL void WRMSR(uint32_t msr, uint64_t value);

ASMCALL void PauseCurrentCore();