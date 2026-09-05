// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>
#include <arch/x86_64/MMD/paging.hpp>
#include <arch/x86_64/MMD/heap.hpp>
#include <arch/x86_64/MMD/virt.hpp>
#include <arch/x86_64/MMD/phys.hpp>

#include <arch/x86_64/Utility/msr.hpp>

#include <arch/x86_64/Interrupts/gdt.hpp>

namespace krnl
{
	extern Paging* paging;
	extern MSR* msr;
	extern PhysAlloc* physAlloc;
	extern VirtAlloc* virtAlloc;
	extern HeapAlloc* heapAlloc;
}