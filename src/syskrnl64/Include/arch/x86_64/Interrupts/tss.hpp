// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>

namespace krnl
{
	struct PACK TSSDesc
	{
		uint16_t limitLow, baseLow;
		uint8_t baseMid, type;
		uint8_t limitHigh, baseHigh;
		uint32_t baseUpper, reserved;
	};

	struct PACK TSS
	{
		uint32_t _Reserved;
		uint64_t rsp0, rsp1, rsp2;

		uint64_t _Reserved1;

		uint64_t ist1, ist2, ist3, ist4, ist5, ist6, ist7;

		uint64_t _Reserved2;

		uint16_t _Reserved3;
		uint16_t ioMapBase;

		uint64_t _Padding[3]; // Pad to 128 bytes
	};

	enum TSSType : uint8_t
	{
		TSS_TYPE_AVAILABLE = 0x09,
		TSS_TYPE_BUSY = 0x0B,

		TSS_DPL_RING1 = (1 << 5),
		TSS_DPL_RING2 = (1 << 6),
		TSS_DPL_RING3 = (1 << 5) | (1 << 6),
		
		TSS_PRESENT = (1 << 7)
	};

}

ASMCALL void LoadTSS(uint16_t tssOffset);

#define CONSTRUCT_TSS(base, limit, type) { \
	(uint16_t)((limit) & 0xFFFF), \
	(uint16_t)((base) & 0xFFFF), \
	(uint8_t)(((base) >> 16) & 0xFF), \
	(uint8_t)(type), \
	(uint8_t)((((limit) >> 16) & 0x0F) | 0x80), \
	(uint8_t)(((base) >> 24) & 0xFF), \
	(uint32_t)(((base) >> 32) & 0xFFFFFFFF), \
	0 \
}

