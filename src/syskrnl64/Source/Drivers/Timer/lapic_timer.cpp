// SPDX-License-Identifier: GPL-3.0-or-later

#include <Drivers/Timer/lapic_timer.hpp>
#include <Drivers/Timer/itsc.hpp>

using namespace SysKrnl64::Timer;

LAPIC_Timer* LAPIC_Timer::instance = nullptr;

bool LAPIC_Timer::Initialize(APIC::APIC* apic, ISR::ISR* isr, MP::LCPU* lcpu)
{
    if(!apic || !isr || !lcpu)
    {
        printf("[SYSKRNL64] [LAPIC TIMER] [ERROR]: Invalid input init parameters\r\n");
        return false;
    }

    this->instance = this;

    this->apic = apic;
    this->isr = isr;
    this->lcpu = lcpu;

    // default init the LT CPU Descriptors

    LT_CPUDesc defDesc = {};
    defDesc.currentMode = LT_Mode::None;
    defDesc.divideConfig = 0x03;
    defDesc.interruptVector = ISR_LAPIC_TIMER;
    defDesc.flags = 0;
    defDesc.frequency = 0;
    defDesc.intervalNS = 0;

    cpuDescs.init(lcpu->GetLPCount(), defDesc);

    // Register the LT Interrupt Handler

    isr->RegisterHandler(ISR_LAPIC_TIMER, InterruptHandler);

    // Mask the timer for the current processor and set the ISR Handler

    apic->WriteLAPIC(LAPIC_LVT_TIMER, ISR_LAPIC_TIMER | 1 << 16);

    return true;
}

// Function must be called on the same LP as the LP's ID specified
bool LAPIC_Timer::InitCPU(MP::LPID cpuID)
{
    if(cpuID >= cpuDescs.size())
    {
        printf("[SYSKRNL64] [LAPIC TIMER] [ERROR]: LP ID %u out of range for a maximum of %u in LAPIC_Timer::InitCPU(MP::LPID cpuID)\r\n", cpuID, cpuDescs.size());
        return false;
    }

    // Setup the descriptor to an initial state

    LT_CPUDesc* cpuDesc = &cpuDescs[cpuID];

    cpuDesc->flags = 0;
    cpuDesc->frequency = 0;
    cpuDesc->divideConfig = 0x03;
    cpuDesc->currentMode = LT_Mode::None;
    cpuDesc->interruptVector = ISR_LAPIC_TIMER;
    cpuDesc->intervalNS = 0;
    
    // Check if TSC Deadline mode is supported

    CPUID::CPUID_Regs regs = CPUID::GetCPUIDInfo(0x01);
    bool tscDeadlineSupported = regs.rcx & (1ULL << 24);

    if(tscDeadlineSupported) cpuDesc->flags |= LT_FLAG_TSC_DEADLINE_SUPPORTED;

    // Mask the timer and set the ISR Handler for the processor the command is being executed on

    apic->WriteLAPIC(LAPIC_LVT_TIMER, cpuDesc->interruptVector | 1 << 16);

    // Write the default divider(0x03: means 16)

    apic->WriteLAPIC(LAPIC_TIMER_DIVIDE, cpuDesc->divideConfig);

    // Clear the old timer state

    apic->WriteLAPIC(LAPIC_TIMER_INIT_COUNT, 0);
    
    return true;
}

bool LAPIC_Timer::Calibrate(MP::LPID cpuID, uint64_t frequency)
{
    if(cpuID >= cpuDescs.size())
    {
        printf("[SYSKRNL64] [LAPIC TIMER] [ERROR]: LP ID %u out of range for a maximum of %u in LAPIC_Timer::Calibrate(MP::LPID cpuID, uint64_t frequency)\r\n", cpuID, cpuDescs.size());
        return false;
    }

    LT_CPUDesc* cpuDesc = &cpuDescs[cpuID];

    cpuDesc->frequency = frequency;

    return true;
}

bool LAPIC_Timer::SetMode(MP::LPID cpuID, LT_Mode mode)
{
    if(cpuID >= cpuDescs.size())
    {
        printf("[SYSKRNL64] [LAPIC TIMER] [ERROR]: LP ID %u out of range for a maximum of %u in LAPIC_Timer::SetMode(MP::LPID cpuID, LT_Mode mode)\r\n", cpuID, cpuDescs.size());
        return false;
    }

    LT_CPUDesc* desc = &cpuDescs[cpuID];

    desc->currentMode = mode;

    uint32_t lapicLVT = apic->ReadLAPIC(LAPIC_LVT_TIMER);

    if(mode == LT_Mode::TSCDeadline)
    {
        // Special handling for TSC Deadline mode

        lapicLVT |= (1 << 18);

        apic->WriteLAPIC(LAPIC_LVT_TIMER, lapicLVT);

        return true;
    }

    // Make sure TSC Deadline flag is cleared
    lapicLVT &= ~(1 << 18);

    uint32_t modeBit = (mode == LT_Mode::Periodic) ? (1 << 17) : 0;

    // Preserve vector & mask, discard everything else
    lapicLVT &= 0x000100FF;
    lapicLVT |= modeBit;

    apic->WriteLAPIC(LAPIC_LVT_TIMER, lapicLVT);

    return true;
}

bool LAPIC_Timer::SetIntervalNS(MP::LPID cpuID, uint64_t ns)
{
    if(cpuID >= cpuDescs.size())
    {
        printf("[SYSKRNL64] [LAPIC TIMER] [ERROR]: LP ID %u out of range for a maximum of %u in LAPIC_Timer::SetIntervalMS(MP::LPID cpuID, uint64_t nanoseconds)\r\n", cpuID, cpuDescs.size());
        return false;
    }

    LT_CPUDesc* desc = &cpuDescs[cpuID];

    if(desc->currentMode == LT_Mode::None)
    {
        printf("[SYSKRNL64] [LAPIC TIMER] [ERROR]: Cannot set the LP %d LAPIC Timer interval without a configured mode\r\n", cpuID);
        return false; 
    }

    desc->intervalNS = ns;
    if(desc->currentMode == LT_Mode::TSCDeadline) return true;

    if(desc->frequency == 0) 
    {
        printf("[SYSKRNL64] [LAPIC TIMER] [ERROR]: Cannot set the interval for the LP %d LAPIC Timer when the timer is not calibrated\r\n", cpuID);
        return false;
    }

    uint128_t ticks128 = (static_cast<uint128_t>(desc->frequency) * ns) / NS_PER_SECOND;
    uint32_t ticks = 0;
    if(ticks128 > UINT32_MAX)
    {
        printf("[SYSKRNL64] [LAPIC TIMER] [WARN]: Interval is too large, setting it to UINT32_MAX\r\n");
        ticks = UINT32_MAX;
    }
    else ticks = static_cast<uint32_t>(ticks128);

    apic->WriteLAPIC(LAPIC_TIMER_INIT_COUNT, ticks);

    return true;
}

bool LAPIC_Timer::SetEvent(MP::LPID cpuID, TimerEventHandler handler)
{
    if(cpuID >= cpuDescs.size())
    {
        printf("[SYSKRNL64] [LAPIC TIMER] [ERROR]: LP ID %u out of range for a maximum of %u in LAPIC_Timer::SetEvent(MP::LPID cpuID, TimerEventHandler handler)\r\n", cpuID, cpuDescs.size());
        return false;
    }

    LT_CPUDesc* desc = &cpuDescs[cpuID];

    if(desc->currentMode == LT_Mode::None)
    {
        printf("[SYSKRNL64] [LAPIC TIMER] [ERROR]: Cannot set the LP %d LAPIC Timer Event without a configured mode\r\n", cpuID);
        return false;
    }

    desc->eventHandler = handler;

    return true;
}

uint32_t LAPIC_Timer::GetCounterValue(MP::LPID cpuID)
{
    if(cpuID >= cpuDescs.size())
    {
        printf("[SYSKRNL64] [LAPIC TIMER] [ERROR]: LP ID %u out of range for a maximum of %u in LAPIC_Timer::GetCounterValue(MP::LPID cpuID)\r\n", cpuID, cpuDescs.size());
        return 0;
    }

    return apic->ReadLAPIC(LAPIC_TIMER_CURRENT_COUNT);
}

bool LAPIC_Timer::Start(MP::LPID cpuID, uint64_t initialNS)
{
    if(cpuID >= cpuDescs.size())
    {
        printf("[SYSKRNL64] [LAPIC TIMER] [ERROR]: LP ID %u out of range for a maximum of %u in LAPIC_Timer::Start(MP::LPID cpuID, uint64_t initialNS)\r\n", cpuID, cpuDescs.size());
        return false;
    }

    LT_CPUDesc* desc = &cpuDescs[cpuID];

    if(desc->currentMode == LT_Mode::None)
    {
        printf("[SYSKRNL64] [LAPIC_TIMER] [ERROR]: Cannot start a LP %d LAPIC Timer without a configured mode\r\n", cpuID);
        return false;
    }

    uint64_t intervalNS = (initialNS == 0) ? desc->intervalNS : initialNS;

    if(desc->currentMode == LT_Mode::TSCDeadline)
    {
        // Start the LAPIC Timer using TSC Deadline mode, and only if invariant TSC is supported
        if(!(desc->flags & LT_FLAG_TSC_DEADLINE_SUPPORTED))
        {
            printf("[SYSKRNL64] [LAPIC TIMER] [ERROR]: TSC Deadline mode is not supported\r\n");
            return false;
        }

        if(!iTSC::isSupported())
        {
            printf("[SYSKRNL64] [LAPIC TIMER] [ERROR]: LAPIC Timer driver doesn't support the use of TSC Deadline mode without invariant TSC support\r\n");
            return false;
        }

        uint64_t tscTicks = static_cast<uint64_t>((static_cast<uint128_t>(iTSC::GetFrequency()) * intervalNS) / NS_PER_SECOND);
        uint64_t deadline = iTSC::GetCounterValue() + tscTicks;

        msr->WriteMSR(MSR_IA32_TSC_DEADLINE, deadline);
        desc->flags |= LT_FLAG_RUNNING;

        return true;
    }

    if(desc->frequency == 0)
    {
        printf("[SYSKRNL64] [LAPIC TIMER] [ERROR]: Cannot start an uncalibrated LAPIC Timer\r\n");
        return false;
    }

    // Calculate the LAPIC Timer ticks from calibrated frequency and start the timer
    uint128_t ticks128 = (static_cast<uint128_t>(desc->frequency) * intervalNS) / NS_PER_SECOND;

    uint32_t ticks;
    if(ticks128 > UINT32_MAX) ticks = UINT32_MAX;
    else ticks = static_cast<uint32_t>(ticks128);
    
    apic->WriteLAPIC(LAPIC_TIMER_INIT_COUNT, ticks);

    // Unmask the LAPIC Timer
    uint32_t LVT = apic->ReadLAPIC(LAPIC_LVT_TIMER);
    LVT &= ~(1 << 16);
    apic->WriteLAPIC(LAPIC_LVT_TIMER, LVT);
    
    desc->flags |= LT_FLAG_RUNNING;
    return true;
}

bool LAPIC_Timer::Stop(MP::LPID cpuID)
{
    if(cpuID >= cpuDescs.size())
    {
        printf("[SYSKRNL64] [LAPIC TIMER] [ERROR]: LP ID %u out of range for a maximum of %u in LAPIC_Timer::Stop(MP::LPID cpuID)\r\n", cpuID, cpuDescs.size());
        return false;
    }

    LT_CPUDesc* desc = &cpuDescs[cpuID];

    if(desc->currentMode == LT_Mode::None)
    {
        printf("[SYSKRNL64] [LAPIC_TIMER] [ERROR]: Cannot stop a LP %d LAPIC Timer without a configured mode\r\n", cpuID);
        return false;
    }

    // Set the mask bit in LVT
    uint32_t LVT = apic->ReadLAPIC(LAPIC_LVT_TIMER);
    LVT |= (1 << 16);

    // Reset the LAPIC Timer and clear the running flag
    apic->WriteLAPIC(LAPIC_LVT_TIMER, LVT);
    apic->WriteLAPIC(LAPIC_TIMER_INIT_COUNT, 0);
    desc->flags &= ~LT_FLAG_RUNNING;

    return true;
}

void LAPIC_Timer::InterruptHandler(ISR::Registers* regs)
{
    // Get current LP LT CPU Desc
    MP::LPSpecificData* lpData = LAPIC_Timer::instance->lcpu->GetLPDataForCurrentLP();
    LT_CPUDesc* cpuDesc = &LAPIC_Timer::instance->cpuDescs[lpData->identity.lpid];

    // If an event handler is present execute it
    if(cpuDesc->eventHandler) cpuDesc->eventHandler(regs);

    // Send EOI
    LAPIC_Timer::instance->apic->SendEOI();
}