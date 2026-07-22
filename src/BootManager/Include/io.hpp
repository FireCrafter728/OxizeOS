// SPDX-License-Identifier: GPL-3.0-or-later
//
// OxizeOS Operating System for the x86 amd64(x86_64) architecture
// Copyright (C) 2025-2026 FireCrafter728
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |-----------------------------------------------------------------------------------------------------------------------------| //
// | OxizeOS Boot Manager Implementation                                                                                         | //
// | IO: Functions with implementations in assembly to use instructions normally not accessible in C/C++ without inline assembly | //
// |-----------------------------------------------------------------------------------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdint.hpp>
#include <SysTable.hpp>

#define ASMCALL extern "C"

void HaltSystem();

ASMCALL void HaltSystemImpl();
ASMCALL void EnableSSE();
ASMCALL uint64_t RDTSC();
ASMCALL void ExecuteKernel(uintptr_t EntryAddr, SystemTable* systemTable, uint64_t CR3, uintptr_t StackAddr);
ASMCALL void* ExecuteKernelEnd();