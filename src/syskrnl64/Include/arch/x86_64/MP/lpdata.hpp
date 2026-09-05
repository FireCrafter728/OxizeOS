// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>

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
		bool InitializeLP(LPID lpId);
		inline LPSpecificData* GetLPDataByID(LPID lpid) { return &lpData[lpid]; }
		inline const stdEx::const_array<LPSpecificData>& GetLPDataArray() const { return lpData; }
		static LPSpecificData* GetLPDataForCurrentLP();
		inline size_t GetLPCount() { return lpData.size(); }
	private:
		stdEx::const_array<LPSpecificData> lpData;
		void StoreCurrentLPData(LPSpecificData* data);
	};
}