// SPDX-License-Identifier: GPL-3.0-or-later

#include <Drivers/Timer/itsc.hpp>

using namespace SysKrnl64::Timer;

bool iTSC::supported;
uint64_t iTSC::frequency;
uint64_t iTSC::ticksAtStartup;            
uint64_t iTSC::ticksToNsMultiplier;

ASMCALL uint64_t RDTSC();

bool iTSC::Initialize()
{
    // Check if the invariant Time Stamp Counter is supported in CPUID
    CPUID::CPUID_Regs regs = CPUID::GetCPUIDInfo(0x80000007);
    iTSC::supported = regs.rdx & (1ULL << 8);
    if(!iTSC::supported) return false;
    return true;
}

bool iTSC::Calibrate(uint64_t frequency)
{
    if(!supported) return false;
    iTSC::frequency = frequency;

    // Calculate TicksToNS Multiplier to optimize the conversion
    ticksToNsMultiplier = (static_cast<uint128_t>(NS_PER_SECOND) << ITSC_CONV_SHIFT) / frequency;

    return true;
}

// if iTSC isn't supported, this will return the regular TSC value
uint64_t iTSC::GetCounterValue()
{
    return RDTSC();
}

uint64_t iTSC::GetNSSinceStartup()
{
    if(!iTSC::supported || iTSC::frequency == 0) return 0;

    uint64_t elapsedTicks = GetCounterValue() - iTSC::ticksAtStartup;

    return (static_cast<uint128_t>(elapsedTicks) * ticksToNsMultiplier) >> ITSC_CONV_SHIFT;
}

void iTSC::SetTimeCalculationTickBase(uint64_t base)
{
    iTSC::ticksAtStartup = base;
}