// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>

#include <API/HandleMgr/handle.hpp>

#include <arch/x86_64/Interrupts/gdt.hpp>
#include <arch/x86_64/Interrupts/idt.hpp>
#include <arch/x86_64/Interrupts/isr.hpp>
#include <arch/x86_64/Interrupts/tss.hpp>

#include <arch/x86_64/ACPI/apic.hpp>

#include <atomic>

namespace krnl
{
	typedef uint32_t LPID; // Global logical processor identifier

	constexpr LPID INVALID_LPID = 0xFFFFFFFF;

	// LP Specific Data

	enum LCPUState : uint16_t
	{
		// Disabled states
		LP_STATE_UNKNOWN = 0,
		LP_STATE_FIRMWARE_DISABLED = 1,
		LP_STATE_UNINITIALIZED = 2,

		// Init states
		LP_STATE_INITIALIZING = 3,

		// Online states
		LP_STATE_ONLINE = 4,
	};

	struct PACK LPIdentifier
	{
		// Core identifiers
		LPID lpid;
		uint32_t apicId;

		// Extra identifiers
		uint32_t packageId, coreId, threadId;
		uint32_t _Padding;
	};

	// Data stored in the GS segment, unique data for each LP
	struct PACK LPSpecificData
	{
		// If changed any of the value locations of the stack variables, update the struct inside arch/x86_64/Interrupts/isr.asm
		LPSpecificData* self; // Used to get quick access of this structure
		LPIdentifier identity;

		// Values for switching the stack 
		uintptr_t ihStackTopPtr;
		uint64_t intDepth;
	};

	static_assert(offsetof(LPSpecificData, self) == 0);

	// LCPU Defs

	struct LPDesc
	{
		LPIdentifier identity;
		LCPUState lpState;
		LPSpecificData* lpSpecificDataPtr;
	};

	constexpr uint16_t LP_STARTUP_STATUS_CPUID_UNSUPPORTED = 0xF001;
	constexpr uint16_t LP_STARTUP_STATUS_LONG_MODE_UNSUPPORTED = 0xF002;

	constexpr uint16_t LP_SPECIFIC_DATA_INIT_FAILED = 0xF101;
	constexpr uint16_t LP_LAPIC_INIT_FAILED = 0xF102;
	constexpr uint16_t LP_INIT_SUCCEEDED = 0xF103;

	constexpr uint16_t LP_STARTUP_STATUS_BOOTSTRAP_COMPLETE = 0xFF00;
	constexpr uint16_t LP_STARTUP_STATUS_EXECUTE_CXX_HANDLER = 0xFF01;

	constexpr uint16_t LP_STARTUP_STATUS_INITIALIZING = 0xFFFF;

	class LPData;

	struct LPInitComponents
	{
		// LP Specific data
		LPData* lpData;

		// GDT Setup
		GDT* gdt;
		GDT_Entry* gdtEntries;
		size_t gdtEntryCount;
		uint16_t gdtCSSelector, gdtDSSelector;

		// IDT setup
		IDT* idt;
		ISR* isr;

		// TSS setup
		uint16_t tssDescOffset;

		// LAPIC Setup
		APIC* apic;
	};

	struct PACK LPStartupHeader
	{
		uint16_t startupStackSegment;
		uint16_t startupStackOffset;
		uint32_t lpId;
		uint64_t CR3Value;
		uintptr_t bootstrapStackTop;
		uintptr_t systemTablePtr;
		uintptr_t cxxHandlerPtr;
		uintptr_t lpInitComponents;
		uint16_t statusCode;
	};

	typedef Handle LPEvent;

	enum LPEventTypes : uint32_t
	{
		LP_EVENT_TYPE_UNKNOWN = 0x00,
		LP_EVENT_TYPE_IDLE = 0x01,
		LP_EVENT_TYPE_EXECUTE = 0x02,

		LP_EVENT_TYPE_INVALIDATE_PAGE = 0x100,
	};

	enum LPEventFlags : uint64_t
	{
		// Bit 0: event contains multiple entries? 0: no, 1: yes
		LP_EVENT_FLAG_MULTIENTRY = (1ULL << 0),
	};

	struct LPEventData
	{
		LPEventTypes eventType;
		LPID targetLPId;
		LPEventFlags eventFlags;

		void* eventData;
		size_t eventDataLength;

		void* returnBuffer;
		size_t returnBufferLength;
		size_t returnBufferElementLength;

		KRNL_STATUS handlerStatus;
		std::atomic<bool> completed;
	};

	typedef KRNL_STATUS (*LPEventFunction)(LPID lpId, SystemTable* System, void* returnBuffer, uint64_t Parameter1, uint64_t Parameter2);
}