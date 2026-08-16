// SPDX-License-Identifier: GPL-3.0-or-later

#include <arch/x86_64/Utility/msr.hpp>
#include <arch/x86_64/Utility/cpuid.hpp>
#include <arch/x86_64/Utility/io.hpp>
#include <stdio.hpp>

using namespace krnl;

bool MSR::Initialize()
{
	CPUID_Regs regs;
	regs = GetCPUIDInfo(1);
	if((regs.rdx & (1 << 5)) == 0) return false;
	present = true;
	return true;
}

uint64_t MSR::ReadMSR(uint32_t msr)
{
	if(!present) return 0xFFFFFFFF;
	return RDMSR(msr);
}

void MSR::WriteMSR(uint32_t msr, uint64_t value)
{
	if(!present) return;
	WRMSR(msr, value);
}