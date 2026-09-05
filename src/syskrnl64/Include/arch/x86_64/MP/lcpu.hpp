// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>

#include <arch/x86_64/ACPI/apic.hpp>
#include <arch/x86_64/ACPI/timer.hpp>

#include <arch/x86_64/MP/smp_defs.hpp>
#include <arch/x86_64/MP/lpdata.hpp>

#include <const_array.hpp>

namespace krnl
{
	struct LCPU_InitDesc
	{
		APIC* apic;
		Timer* timer;
		LPData* lpData;
		SystemTable* System;

		IDT* idt;
		ISR* isr;
	};

	enum LCPUHandleTypes : uint8_t // 256 entries per page
	{
		LCPU_HANDLE_TYPE_EVENT = 0,
	};

	class LCPU
	{
	public:
		KRNL_STATUS Initialize(LCPU_InitDesc* initDesc);
		KRNL_STATUS EnterEventHandler(LPEvent initialEvent, SystemTable* System);
		std::expected<LPEvent, KRNL_STATUS> CreateEvent(LPEventData* eventData);
		KRNL_STATUS ExecuteEvent(LPEvent event, bool waitUntilFinish);
		KRNL_STATUS DestroyEvent(LPEvent event);
	private:
		LCPU_InitDesc* initDesc;
		stdEx::const_array<LPDesc> lpDescs;
		uint64_t eventTypePage;
		LPID bspId;

		KRNL_STATUS SetupLPInitContext(LPDesc* lpDesc, LPInitComponents* initComponents);
		void SendINIT_IPI(LPDesc* lpDesc);
		void SendStartupIPI(LPDesc* lpDesc);
		void SendWakeupIPI(LPDesc* lpDesc);
	};
}