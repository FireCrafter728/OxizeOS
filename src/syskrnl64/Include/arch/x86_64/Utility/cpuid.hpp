// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>

namespace krnl
{
	struct CPUID_Regs
	{
		uint64_t rax, rbx, rcx, rdx;
	};

	CPUID_Regs GetCPUIDInfo(uint32_t leaf, uint32_t subLeaf = 0);
}