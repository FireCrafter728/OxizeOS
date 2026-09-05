// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>

namespace krnl
{
	constexpr uint8_t ITSC_CONV_SHIFT = 48;

	class iTSC
	{
	public:
		static bool Initialize();
		static bool Calibrate(uint64_t frequency);

		static uint64_t GetCounterValue();
		static uint64_t GetNSSinceStartup();

		static void SetTimeCalculationTickBase(uint64_t base);

		static inline uint64_t GetFrequency() { return frequency; }

		static inline bool isSupported() { return iTSC::supported; }
	private:
		static bool supported;
		static uint64_t frequency;
		static uint64_t ticksAtStartup;
		
		static uint64_t ticksToNsMultiplier;
	};
}