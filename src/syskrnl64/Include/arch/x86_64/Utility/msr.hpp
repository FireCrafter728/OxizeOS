// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>
#include <arch/x86_64/std/stdint.hpp>

#define MSR_APIC_BASE 0x1B
#define MSR_X2APIC_BASE 0x800
#define MSR_IA32_TSC_DEADLINE 0x6E0
#define MSR_IA32_FS_BASE 0xC0000100
#define MSR_IA32_GS_BASE 0xC0000101

namespace krnl
{
	class MSR
	{
	public:
		MSR() = default;
		bool Initialize();
		uint64_t ReadMSR(uint32_t msr);
		void WriteMSR(uint32_t msr, uint64_t value);
	private:
		bool present = false;
	};
}