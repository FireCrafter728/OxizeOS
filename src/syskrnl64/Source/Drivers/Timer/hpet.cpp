// SPDX-License-Identifier: GPL-3.0-or-later

#include <Drivers/Timer/hpet.hpp>

using namespace SysKrnl64::Timer;

bool HPET_Timer::Initialize(HPETDevice* device)
{
    if(!device || !device->hpet)
    {
        printf("[SYSKRNL64] [HPET] [ERROR]: Invalid input init parameters\r\n");
        return false;
    }

    if(device->hpet->address.addressSpaceID != 0) // The HPET Regs must me memory mapped, not trough I/O ports
    {
        printf("[SYSKRNL64] [HPET] [ERROR]: HPET Device is reported to use I/O ports, which is not valid and may indicate firmware corruption\r\n");
        return false;
    }

    // Map the HPET Registers
    // The register structure is 1024 bytes, so 1 page should be enough

    auto regAllocRes = virtAlloc->AllocateBlocks(1, MMD::VA_NODE_FLAG_USED | MMD::VA_NODE_FLAG_MMIO | MMD::VA_NODE_FLAG_NO_EXECUTE_ACCESS);
    if(!regAllocRes)
    {
        printf("[SYSKRNL64] [HPET] [ERROR]: Failed to allocate virtual memory for HPET Registers, error code: %u\r\n", regAllocRes.error());
        return false;
    }

    paging->MapArea(device->hpet->address.address, reinterpret_cast<uintptr_t>(regAllocRes.value()), 1, PTE_PRESENT | PTE_RW | PTE_PCD | PTE_NX);
    device->regs = reinterpret_cast<HPETRegisters*>(regAllocRes.value());

    // Parse the general Capabilities and ID register

    printf("[SYSKRNL64] [HPET] [INFO]: Initializing HPET Device, revision: %u\r\n", (device->regs->generalCapAndID & HPET_CAP_REVISION_ID_MASK));

    device->timerCount = ((device->regs->generalCapAndID & HPET_CAP_TIMER_COUNT_MASK) >> HPET_CAP_TIMER_COUNT_SHIFT) + 1;

    device->is64bitCapable = (device->regs->generalCapAndID & HPET_CAP_COUNTER_SIZE_MASK);

    device->vendorID = (device->regs->generalCapAndID & HPET_CAP_VENDOR_ID_MASK) >> HPET_CAP_VENDOR_ID_SHIFT;

    device->frequency = FS_PER_SECOND / ((device->regs->generalCapAndID & HPET_CAP_CLOCK_PERIOD_MASK) >> HPET_CAP_CLOCK_PERIOD_SHIFT);

    device->lastUsedTimer = 0;

    // Calculate the ticksToNsMultiplier for the GetNSSinceStartup function to avoid multiple divisions

    device->ticksToNsMultiplier = (static_cast<uint128_t>(NS_PER_SECOND) << HPET_CONV_SHIFT) / device->frequency;
    device->nsToTicksMultiplier = (static_cast<uint128_t>(device->frequency) << HPET_CONV_SHIFT) / NS_PER_SECOND;

    // Convert frequency number to string for output

    char strBuffer[64];
    size_t bufferSize = 64;

    if(!stdEx::convertFreqToStr(device->frequency, strBuffer, &bufferSize)) 
    {
        printf("[SYSKRNL64] [HPET] [ERROR]: Frequency formatting failed\r\n");
        return false;
    }

    printf("[SYSKRNL64] [HPET] [INFO]: HPET Device specs: vendor ID: 0x%X, mode: %s, timer count: %u, frequency: %s\r\n", device->vendorID, device->is64bitCapable ? "64-bit" : "32-bit", device->timerCount, strBuffer);

    // Temporarily disable the HPET device

    device->regs->generalConfig &= ~(HPET_CONFIG_ENABLE_MASK);

    // Disable all timers

    if(device->timerCount >= 1) 
    {
        device->regs->timer0.ConfigAndCap &= ~HPET_TIMER_CFG_INT_ENABLE_MASK;
        device->regs->generalInterruptStatus = 1ULL << 0;
    }
    if(device->timerCount >= 2)
    {
        device->regs->timer1.ConfigAndCap &= ~HPET_TIMER_CFG_INT_ENABLE_MASK;
        device->regs->generalInterruptStatus = 1ULL << 1;
    }
    if(device->timerCount >= 3)
    {
        device->regs->timer2.ConfigAndCap &= ~HPET_TIMER_CFG_INT_ENABLE_MASK;
        device->regs->generalInterruptStatus = 1ULL << 2;
    }

    if(device->timerCount > 3)
    {
        for(size_t i = 0; i < device->timerCount - 3; i++)
        {
            device->regs->timers3_31[i].ConfigAndCap &= ~HPET_TIMER_CFG_INT_ENABLE_MASK;
            device->regs->generalInterruptStatus = 1ULL << (i + 3);
        }
    }

    // Reset and enable main counter

    if(!RestartMainCounter(device))
    {
        printf("[SYSKRNL64] [HPET] [ERROR]: Failed to restart main counter\r\n");
        return false;
    }

    return true;
}

bool HPET_Timer::SetupTimer(HPETDevice* device, HPETTimer* timerOut, uint8_t* isrOut, bool periodic)
{
    if(!device || !device->apic || !device->regs || !timerOut)
    {
        printf("[SYSKRNL64] [HPET] [ERROR]: Invalid SetupTimer input parameters\r\n");
        return false;
    }

    if(device->lastUsedTimer >= device->timerCount)
    {
        printf("[SYSKRNL64] [HPET] [ERROR]: No more available HPET Timers\r\n");
        return false;
    }

    // Select a free timer

    if(device->lastUsedTimer == 0) 
    {
        timerOut->config = &device->regs->timer0;
        timerOut->timerIndex = 0;
    }
    else if(device->lastUsedTimer == 1)
    {
        timerOut->config = &device->regs->timer1;
        timerOut->timerIndex = 1;
    }
    else if(device->lastUsedTimer == 2) 
    {
        timerOut->config = &device->regs->timer2;
        timerOut->timerIndex = 2;
    }
    else 
    {
        timerOut->config = &device->regs->timers3_31[device->lastUsedTimer - 3];
        timerOut->timerIndex = device->lastUsedTimer;
    }

    // Setup timer configs

    timerOut->irqLine = 0xFF;

    volatile uint64_t* cfg = reinterpret_cast<volatile uint64_t*>(&timerOut->config->ConfigAndCap);
    uint64_t cap = *cfg;

    cap &= ~HPET_TIMER_CFG_INT_ENABLE_MASK;
    if(periodic) cap |= HPET_TIMER_CFG_TYPE_MASK;
    else cap &= ~HPET_TIMER_CFG_TYPE_MASK;
    cap |= HPET_TIMER_CFG_PERIODIC_ACCUM_MASK;
    cap &= ~HPET_TIMER_CFG_FSB_ENABLE_MASK;
    cap &= ~HPET_TIMER_CFG_INT_TYPE_MASK;

    // Assign an IOAPIC GSI to the timer and return the assigned ISR to the caller

    uint64_t routeCapabilities = cap >> HPET_TIMER_CFG_ROUTE_CAP_SHIFT;

    uint8_t selectedGSI = 0xFF;
    uint8_t isr = 0;

    for(uint8_t gsi = 0; gsi < 32; gsi++)
    {
        if(routeCapabilities & (1ULL << gsi))
        {
            isr = device->apic->AllocateSpecificGSI(gsi, APIC::IOAPIC_TRIGGER_EDGE | APIC::IOAPIC_TRIGGER_HIGH);

            if(isr != 0)
            {
                selectedGSI = gsi;
                break;
            }
        }
    }

    if(selectedGSI == 0xFF)
    {
        printf("[SYSKRNL64] [HPET] [ERROR]: Failed to allocate a supported HPET interrupt route\r\n");
        return false;
    }

    timerOut->irqLine = selectedGSI;

    cap &= ~HPET_TIMER_CFG_INT_ROUTING_MASK;
    cap |= (static_cast<uint64_t>(selectedGSI) << HPET_TIMER_CFG_INT_ROUTING_SHIFT);

    *cfg = cap;
    *isrOut = isr;

    device->lastUsedTimer++;

    return true;
}

bool HPET_Timer::ArmTimer(HPETDevice* device, HPETTimer* timer, uint64_t targetNs, uint64_t counterBase)
{
    if(!device || !device->regs || !timer)
    {
        printf("[SYSKRNL64] [HPET] [ERROR]: Invalid ArmTimer input parameters\r\n");
        return false;
    }

    // Disable interrupts while arming

    timer->config->ConfigAndCap &= ~HPET_TIMER_CFG_INT_ENABLE_MASK;

    // Calculate total ticks

    uint64_t ticks = static_cast<uint64_t>((static_cast<uint128_t>(targetNs) * device->nsToTicksMultiplier) >> HPET_CONV_SHIFT);
    if(counterBase == 0) ticks += device->regs->mainCounterValue;
    else ticks += counterBase;

    timer->config->ComparatorValueRegister = ticks;

    // Clear the interrupt status bit for this timer

    device->regs->generalInterruptStatus = (1ULL << timer->timerIndex);

    // Allow interrupts

    timer->config->ConfigAndCap |= HPET_TIMER_CFG_INT_ENABLE_MASK;

    return true;
}

bool HPET_Timer::StopTimer(HPETDevice* device, HPETTimer* timer)
{
    if(!device || !device->regs || !timer)
    {
        printf("[SYSKRNL64] [HPET] [ERROR]: Invalid StopTimer input parameters\r\n");
        return false;
    }

    timer->config->ConfigAndCap &= ~HPET_TIMER_CFG_INT_ENABLE_MASK;

    return true;
}

uint64_t HPET_Timer::GetCounterTicks(HPETDevice* device)
{
    return device->regs->mainCounterValue;
}

uint64_t HPET_Timer::GetCounterFrequency(HPETDevice* device)
{
    return device->frequency;
}

bool HPET_Timer::ResetMainCounter(HPETDevice* device)
{
    if(!device)
    {
        printf("[SYSKRNL64] [HPET] [ERROR]: Invalid ResetMainCounter input parameters\r\n");
        return false;
    }
    device->regs->mainCounterValue = 0;
    return true;
}

bool HPET_Timer::StartMainCounter(HPETDevice* device)
{
    if(!device)
    {
        printf("[SYSKRNL64] [HPET] [ERROR]: Invalid StartMainCounter input parameters\r\n");
        return false;
    }
    device->regs->generalConfig |= HPET_CONFIG_ENABLE_MASK;
    return true;
}

bool HPET_Timer::StopMainCounter(HPETDevice* device)
{
    if(!device)
    {
        printf("[SYSKRNL64] [HPET] [ERROR]: Invalid StopMainCounter input parameters\r\n");
        return false;
    }
    device->regs->generalConfig &= ~HPET_CONFIG_ENABLE_MASK;
    return true;
}

bool HPET_Timer::RestartMainCounter(HPETDevice* device)
{
    if(!device)
    {
        printf("[SYSKRNL64] [HPET] [ERROR]: Invalid RestartMainCounter input parameters\r\n");
        return false;
    }
    if(!StopMainCounter(device)) return false;
    if(!ResetMainCounter(device)) return false;
    if(!StartMainCounter(device)) return false;
    return true;
}

uint64_t HPET_Timer::GetNSSinceStartup(HPETDevice* device)
{
    if(!device)
    {
        printf("[SYSKRNL64] [HPET] [ERROR]: Invalid GetNSSinceStartup input parameters\r\n");
        return false;
    }

    return static_cast<uint64_t>(static_cast<uint128_t>(GetCounterTicks(device)) * device->ticksToNsMultiplier) >> HPET_CONV_SHIFT;
}