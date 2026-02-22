#pragma once

#include <stdint.hpp>
#include <SysTable.hpp>

#define ASMCALL extern "C"

void HaltSystem();

ASMCALL void HaltSystemImpl();
ASMCALL void EnableSSE();
ASMCALL void TestInt();
ASMCALL void ExecuteKernel(uintptr_t EntryAddr, SystemTable* systemTable, uint64_t CR3, uintptr_t StackAddr);
ASMCALL void* ExecuteKernelEnd();