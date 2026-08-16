// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <defs.hpp>
#include <SysTable.hpp>

#define ASMCALL extern "C"

void HaltSystem();

ASMCALL void HaltSystemImpl();
ASMCALL void EnableSSE();
ASMCALL uint64_t RDTSC();
ASMCALL void ExecuteKernel(uintptr_t EntryAddr, SystemTable* systemTable, uint64_t CR3, uintptr_t StackAddr);
ASMCALL void* ExecuteKernelEnd();