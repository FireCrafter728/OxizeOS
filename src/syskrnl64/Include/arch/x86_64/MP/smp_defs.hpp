// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>
#include <arch/x86_64/std/stdint.hpp>
#include <arch/x86_64/std/stddef.hpp>

namespace krnl
{
	typedef uint32_t LPID; // Global logical processor identifier

	constexpr LPID INVALID_LPID = 0xFFFFFFFF;

	enum LCPUState : uint16_t
	{
		LP_STATE_UNKNOWN = 0,
		LP_STATE_FIRMWARE_DISABLED = 1,
		LP_STATE_UNINITIALIZED = 2,
		LP_STATE_INITIALIZING = 3,
		LP_STATE_ONLINE = 4,
		LP_STATE_HALTED = 5,
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
		LPSpecificData* self; // Used to get quick access of this structure
		LPIdentifier identity;

		// Values for switching the stack 
		uintptr_t ihStackTopPtr;
		uintptr_t ihStackBottomPtr;
		uint64_t intDepth;
	};

	static_assert(offsetof(LPSpecificData, self) == 0);
}