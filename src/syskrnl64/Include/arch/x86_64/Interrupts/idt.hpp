// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>
#include <arch/x86_64/std/stdint.hpp>

namespace krnl
{
	struct PACK IDT_Entry
	{
		uint16_t BaseLow;
		uint16_t SegmentSelector;
		uint8_t IST;
		uint8_t Flags;
		uint16_t BaseMiddle;
		uint32_t BaseHigh;
		uint32_t Reserved;
	};

	struct FPACK IDT_Desc
	{
		uint16_t Limit;
		IDT_Entry* entries;
	};

	enum IDT_FLAGS : uint8_t
	{
		IDT_FLAG_GATE_TASK = 0x05,
		IDT_FLAG_GATE_16BIT_INT = 0x06,
		IDT_FLAG_GATE_16BIT_TRAP = 0x07,
		
		IDT_FLAG_GATE_32BIT_INT = 0x0E,
		IDT_FLAG_GATE_32BIT_TRAP = 0x0F,

		IDT_FLAG_GATE_64BIT_INT = 0x0E,
		IDT_FLAG_GATE_64BIT_TRAP = 0x0F,

		IDT_FLAG_RING0 = 0x00,
		IDT_FLAG_RING1 = 0x20,
		IDT_FLAG_RING2 = 0x40,
		IDT_FLAG_RING3 = 0x60,

		IDT_FLAG_PRESENT = 0x80,
	};

	class IDT
	{
	public:
		void Initialize();
		static void SetGate(int interrupt, void* base, uint16_t segmentDescriptor, uint8_t flags, uint8_t interruptIST = 0);
		static void EnableGate(int interrupt);
		static void DisableGate(int interrupt);
	private:
		IDT_Desc idtDesc;
		static IDT_Entry entries[256];
	};
}

constexpr uint8_t IDT_FLAG_GATE_TASK 		= krnl::IDT_FLAG_GATE_TASK;
constexpr uint8_t IDT_FLAG_GATE_16BIT_INT 	= krnl::IDT_FLAG_GATE_16BIT_INT;
constexpr uint8_t IDT_FLAG_GATE_16BIT_TRAP 	= krnl::IDT_FLAG_GATE_16BIT_TRAP;
constexpr uint8_t IDT_FLAG_GATE_32BIT_INT 	= krnl::IDT_FLAG_GATE_32BIT_INT;
constexpr uint8_t IDT_FLAG_GATE_32BIT_TRAP 	= krnl::IDT_FLAG_GATE_32BIT_TRAP;
constexpr uint8_t IDT_FLAG_GATE_64BIT_INT 	= krnl::IDT_FLAG_GATE_64BIT_INT;
constexpr uint8_t IDT_FLAG_GATE_64BIT_TRAP 	= krnl::IDT_FLAG_GATE_64BIT_TRAP;
constexpr uint8_t IDT_FLAG_RING0 			= krnl::IDT_FLAG_RING0;
constexpr uint8_t IDT_FLAG_RING1 			= krnl::IDT_FLAG_RING1;
constexpr uint8_t IDT_FLAG_RING2 			= krnl::IDT_FLAG_RING2;
constexpr uint8_t IDT_FLAG_RING3 			= krnl::IDT_FLAG_RING3;
constexpr uint8_t IDT_FLAG_PRESENT 			= krnl::IDT_FLAG_PRESENT;