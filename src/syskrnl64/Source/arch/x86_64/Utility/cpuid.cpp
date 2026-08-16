// SPDX-License-Identifier: GPL-3.0-or-later

#include <arch/x86_64/Utility/cpuid.hpp>
#include <arch/x86_64/Utility/io.hpp>

krnl::CPUID_Regs krnl::GetCPUIDInfo(uint32_t leaf, uint32_t subLeaf)
{
	CPUID_Regs regs;
	GetCPUID(leaf, subLeaf, &regs);
	return regs;
}