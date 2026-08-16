// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>
#include <arch/x86_64/std/stdint.hpp>
#include <arch/x86_64/std/stddef.hpp>

extern const uint16_t GDT_64BIT_RING0_CODESEG;
extern const uint16_t GDT_64BIT_RING0_DATASEG;
extern const uint16_t GDT_64BIT_RING3_CODESEG;
extern const uint16_t GDT_64BIT_RING3_DATASEG;

extern const uint16_t GDT_32BIT_RING0_CODESEG;
extern const uint16_t GDT_32BIT_RING0_DATASEG;
extern const uint16_t GDT_32BIT_RING3_CODESEG;
extern const uint16_t GDT_32BIT_RING3_DATASEG;

namespace krnl
{
	struct PACK GDT_Entry
	{
		uint16_t LimitLow;
		uint16_t BaseLow;
		uint8_t BaseMiddle;
		uint8_t Access;
		uint8_t FlagsLimitHigh;
		uint8_t BaseHigh;
	};

	struct FPACK GDT_Desc
	{
		uint16_t Limit;
		GDT_Entry* entries;
	};

	enum GDT_ACCESS : uint8_t
	{
		GDT_ACCESS_CODE_READABLE = 0x02,
		GDT_ACCESS_DATA_WRITEABLE = 0x02,

		GDT_ACCESS_CODE_CONFORMING = 0x04,
		GDT_ACCESS_DATA_DIRECTION_NORMAL = 0x00,
		GDT_ACCESS_DATA_DIRECTION_DOWN = 0x04,

		GDT_ACCESS_CODE_SEGMENT = 0x18,
		GDT_ACCESS_DATA_SEGMENT = 0x10,

		GDT_ACCESS_DESCRIPTOR_TSS = 0x00,
		
		GDT_ACCESS_RING0 = 0x00,
		GDT_ACCESS_RING1 = 0x20,
		GDT_ACCESS_RING2 = 0x40,
		GDT_ACCESS_RING3 = 0x60,

		GDT_ACCESS_PRESENT = 0x80,
	};

	enum GDT_FLAGS : uint8_t
	{
		GDT_FLAG_64BIT = 0x20,
		GDT_FLAG_32BIT = 0x40,
		GDT_FLAG_16BIT = 0x00,
		
		GDT_FLAG_GRANULARITY_1B = 0x00,
		GDT_FLAG_GRANULARITY_4K = 0x80,
	};

	class GDT
	{
	public:
		void Initialize(GDT_Entry* entries, size_t entryCount, uint16_t newCs, uint16_t newDs);
	private:
		GDT_Desc gdtDesc;
	};
}

constexpr uint8_t GDT_ACCESS_CODE_READABLE 			= krnl::GDT_ACCESS_CODE_READABLE;
constexpr uint8_t GDT_ACCESS_DATA_WRITEABLE 		= krnl::GDT_ACCESS_DATA_WRITEABLE;
constexpr uint8_t GDT_ACCESS_CODE_CONFORMING 		= krnl::GDT_ACCESS_CODE_CONFORMING;
constexpr uint8_t GDT_ACCESS_DATA_DIRECTION_NORMAL 	= krnl::GDT_ACCESS_DATA_DIRECTION_NORMAL;
constexpr uint8_t GDT_ACCESS_DATA_DIRECTION_DOWN 	= krnl::GDT_ACCESS_DATA_DIRECTION_DOWN;
constexpr uint8_t GDT_ACCESS_CODE_SEGMENT 			= krnl::GDT_ACCESS_CODE_SEGMENT;
constexpr uint8_t GDT_ACCESS_DATA_SEGMENT 			= krnl::GDT_ACCESS_DATA_SEGMENT;
constexpr uint8_t GDT_ACCESS_DESCRIPTOR_TSS			= krnl::GDT_ACCESS_DESCRIPTOR_TSS;
constexpr uint8_t GDT_ACCESS_RING0 					= krnl::GDT_ACCESS_RING0;
constexpr uint8_t GDT_ACCESS_RING1 					= krnl::GDT_ACCESS_RING1;
constexpr uint8_t GDT_ACCESS_RING2 					= krnl::GDT_ACCESS_RING2;
constexpr uint8_t GDT_ACCESS_RING3 					= krnl::GDT_ACCESS_RING3;
constexpr uint8_t GDT_ACCESS_PRESENT 				= krnl::GDT_ACCESS_PRESENT;

constexpr uint8_t GDT_FLAG_64BIT 					= krnl::GDT_FLAG_64BIT;
constexpr uint8_t GDT_FLAG_32BIT 					= krnl::GDT_FLAG_32BIT;
constexpr uint8_t GDT_FLAG_16BIT 					= krnl::GDT_FLAG_16BIT;
constexpr uint8_t GDT_FLAG_GRANULARITY_1B 			= krnl::GDT_FLAG_GRANULARITY_1B;
constexpr uint8_t GDT_FLAG_GRANULARITY_4K 			= krnl::GDT_FLAG_GRANULARITY_4K;

#define GDT_LIMIT_LOW(limit)                ((limit) & 0xFFFF)
#define GDT_BASE_LOW(base)                  ((base) & 0xFFFF)
#define GDT_BASE_MIDDLE(base)               (((base) >> 16) & 0xFF)
#define GDT_FLAGS_LIMIT_HIGH(limit, flags)  ((((limit) >> 16) & 0xF) | ((flags) & 0xF0))
#define GDT_BASE_HIGH(base)                 (((base) >> 24) & 0xFF)

#define GDT_ENTRY(base, limit, access, flags) { \
	GDT_LIMIT_LOW(limit),                       \
	GDT_BASE_LOW(base),                         \
	GDT_BASE_MIDDLE(base),                      \
	(access),                                     \
	GDT_FLAGS_LIMIT_HIGH(limit, flags),         \
	GDT_BASE_HIGH(base)                         \
}

