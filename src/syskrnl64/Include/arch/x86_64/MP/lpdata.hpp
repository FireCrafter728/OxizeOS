// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>
#include <arch/x86_64/std/stdint.hpp>
#include <const_array.hpp>

#include <arch/x86_64/MP/smp_defs.hpp>

#include <arch/x86_64/ACPI/apic.hpp>

namespace krnl
{
	class LPData
	{
	public:
		bool InitializeBSP(LPSpecificData* dataOut);
		bool Initialize(APIC* apic, LPSpecificData* bspData);
		inline LPSpecificData* GetLPDataByID(LPID lpid) { return &lpData[lpid]; }
		LPSpecificData* GetLPDataForCurrentLP();
		inline size_t GetLPCount() { return lpData.size(); }
	private:
		stdEx::const_array<LPSpecificData> lpData;
		void StoreCurrentLPData(LPSpecificData* data);
	};
}