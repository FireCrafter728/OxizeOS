// SPDX-License-Identifier: GPL-3.0-or-later

#include <arch/x86_64/ACPI/timer.hpp>
#include <arch/x86_64/Interrupts/isr_mappings.hpp>
#include <arch/x86_64/Utility/cpuid.hpp>
#include <arch/x86_64/Interrupts/idt.hpp>
#include <arch/x86_64/Utility/io.hpp>

#include <stdio.hpp>
#include <converter.hpp>

using namespace krnl;

Timer* Timer::instance = nullptr;

uint64_t iTSCCalibrationTimestamp = 0;
static void iTSCCalibrationEvent(ISR_InterruptStackFrame*);

bool sleepDone = false;
static void SleepEvent(ISR_InterruptStackFrame*);

bool Timer::Initialize(const TimerDesc* desc)
{
	if(!desc || !desc->irq || !desc->hpet || !desc->hpetDevice || !desc->System)
	{
		printf("[SYSKRNL64] [TIMER] [ERROR]: Invalid Initialize() input parameters\r\n");
		return false;
	}

	this->desc = desc;
	this->instance = this;

	// Setup a HPET Timer and Register HPET Interrupt handler

	if(!desc->hpet->SetupTimer(desc->hpetDevice, &hpetTimer, &hpetIsr, false))
	{
		printf("[SYSKRNL64] [TIMER] [ERROR]: Failed to setup HPET Timer\r\n");
		return false;
	}

	if(!desc->irq->RegisterHandler(hpetIsr - IRQ_BASE, HPET_InterruptHandler))
	{
		printf("[SYSKRNL64] [TIMER] [ERROR]: Failed to register HPET Interrupt handler\r\n");
		return false;
	}

	// Check if the iTSC is supported
	
	if(!iTSC::isSupported())
	{
		printf("[SYSKRNL64] [TIMER] [WARN]: Invariant Time Stamp Counter not supported, falling back to High Precision Event Timer\r\n");
		primaryTimer = Timer_PrimaryTimer::HPET;
	}
	else primaryTimer = Timer_PrimaryTimer::iTSC;

	// Before calibrating the iTSC setup the current timestamp for system time related functions

	// if iTSC is supported, set the time calculation base to the tsc value during the GetTime call to the firmware, stored in the system table

	if(primaryTimer == Timer_PrimaryTimer::iTSC)
	{
		iTSC::SetTimeCalculationTickBase(desc->System->bootTime.tscCounter);
		initTime = desc->System->bootTime.systemTime;
	}
	else
	{
		// iTSC isn't supported, so we'll have to accept the delay that happened between firmare GetTime call and RestartMainCounter call. It shouldn't be over a few ms, usually below a millisecond

		initTime = desc->System->bootTime.systemTime;
		desc->hpet->RestartMainCounter(desc->hpetDevice);
		return true;
	}

	// Calibrate the iTSC

	uint64_t iTSCFrequency = 0;

	// Before calibrating manually, see if the CPU exposed the iTSC frequency in CPUID leaf 0x15
	// By intel spec, the iTSC properties can be specified in CPUID leaf 0x15, but not all CPUs populate that field, so we use it as a hint, not as the only source

	// First, check if that leaf even exists

	bool leafExists = false;
	CPUID_Regs maxLeaf = GetCPUIDInfo(0x0);
	if(maxLeaf.rax >= 0x15)
	{
		leafExists = true;
		// Leaf supported, check if the fields are populated
		CPUID_Regs iTSCLeaf = GetCPUIDInfo(0x15);
		if(iTSCLeaf.rax != 0 && iTSCLeaf.rbx != 0 && iTSCLeaf.rcx != 0) iTSCFrequency = (static_cast<uint128_t>(iTSCLeaf.rcx) * iTSCLeaf.rbx) / iTSCLeaf.rax;
	}

	if(iTSCFrequency == 0)
	{
		if(!leafExists) printf("[SYSKRNL64] [TIMER] [INFO]: Cannot use CPUID Leaf 0x15 to get iTSC frequency, calculating it manually\r\n");
		else printf("[SYSKRNL64] [TIMER] [INFO]: CPUID Leaf 0x15 isn't populated, calculating the iTSC frequency manually\r\n");
		
		// Manual calibration sequence:
		// Assign an itsc calibration event to be called by the hpet
		// Setup the HPET to interrupt after 100ms, halt the current code(with interrupts enabled)
		// After 100ms passes, in the itsc calibration event store the iTSC counter value
		// When iretq is executed, the main code should be unhalted and continue executing
		// Then calculate how much time passed since the hpet arm and the counter value store
		// Multiply by 10 to get the frequency in hz and use that to call iTSC::Calibrate, which expects frequency

		// Assign the event
		this->hpetEvent = iTSCCalibrationEvent;

		// Arm the HPET for 100ms
		if(!desc->hpet->ArmTimer(desc->hpetDevice, &hpetTimer, NS_PER_MILLISECOND * 100))
		{
			printf("[SYSKRNL64] [TIMER] [ERROR]: Failed to arm the HPET Timer for calibrating the iTSC\r\n");
			return false;
		}
		uint64_t timestamp = iTSC::GetCounterValue();

		while(iTSCCalibrationTimestamp == 0) SuspendCurrentCore();

		// At this point the second timestamp is set, calculate the elapsed ticks, convert to frequency and call iTSC::Calibrate
		uint64_t elapsedTicks = iTSCCalibrationTimestamp - timestamp;
		iTSCFrequency = elapsedTicks * 10; // Convert to frequency, ticks per 100ms * 10 = ticks per 1s = frequency
		desc->hpet->StopTimer(desc->hpetDevice, &hpetTimer);
	}

	if(!iTSC::Calibrate(iTSCFrequency))
	{
		printf("[SYSKRNL64] [TIMER] [ERROR]: Failed to calibrate iTSC\r\n");
		return false;
	}

	// Format and print the iTSC Frequency

	char strBuffer[64];
	size_t bufferSize = 64;
	if(!stdEx::convertFreqToStr(iTSCFrequency, strBuffer, &bufferSize)) 
	{
		printf("[SYSKRNL64] [TIMER] [ERROR]: Frequency formatting failed\r\n");
		return false;
	}

	printf("[SYSKRNL64] [TIMER] [INFO]: Successfully calibrated the invariant TSC, frequency: %s\r\n", strBuffer);

	return true;
}

void Timer::HPET_InterruptHandler(ISR_InterruptStackFrame* regs)
{
	if(!instance->hpetEvent)
	{
		printf("[SYSKRNL64] [TIMER] [WARN]: HPET Interrupt occured with no registered event handler\r\n");
		return;
	}

	instance->hpetEvent(regs);

	instance->desc->hpet->StopTimer(instance->desc->hpetDevice, &instance->hpetTimer);
}

static void iTSCCalibrationEvent(ISR_InterruptStackFrame*)
{
	iTSCCalibrationTimestamp = iTSC::GetCounterValue();
}

SystemTime Timer::GetSystemTime()
{
	// Make a copy of initTime
	SystemTime copy = initTime;
	uint64_t nanosecondsPassed = 0;

	if(primaryTimer == Timer_PrimaryTimer::iTSC)
	{
		// Primary timer is the time stamp counter, use it's GetNSSinceStartup()
		nanosecondsPassed = iTSC::GetNSSinceStartup();
	}
	else
	{
		// Primary timer is the High Precision Event Timer, use it's GetNSSinceStartup()
		nanosecondsPassed = desc->hpet->GetNSSinceStartup(desc->hpetDevice);
	}

	nanosecondsPassed += copy.Nanoseconds;

	copy.SecondsSinceEpoch += nanosecondsPassed / NS_PER_SECOND;
	copy.Nanoseconds = nanosecondsPassed % NS_PER_SECOND;

	return copy;
}

void Timer::SleepMS(uint64_t milliseconds)
{
	SleepNS(milliseconds * NS_PER_MILLISECOND);
}

void Timer::SleepNS(uint64_t nanoseconds)
{
	if(nanoseconds < TIMER_SLEEP_BUSYWAIT_TRESHOLD)
	{
		// Sleep time is below the treshold, use a busy wait loop with the CPU `pause` instruction and check if the elapsed

		if(primaryTimer == Timer_PrimaryTimer::iTSC)
		{
			// Using the iTSC for the busy wait loop
			// Calculate at what tick should the busy wait loop be done

			uint64_t currentTicks = iTSC::GetCounterValue();
			uint64_t frequency = iTSC::GetFrequency();

			uint64_t ticksToWait = (static_cast<uint128_t>(nanoseconds) * frequency) / NS_PER_SECOND;
			uint64_t deadline = currentTicks + ticksToWait;

			// busy wait loop
			while(iTSC::GetCounterValue() < deadline)
			{
				Pause();
			}

			// Counter value has surpassed the deadline, return
			return;
		}
		else
		{
			// Using the HPET for the busy wait loop
			// If the HPET Counter is 32-bit the deadline might never be reached and the system might be in a forever loop
			// This might be fixed in the future updates, and should be fixed before the first kernel release

			// Calculate at what tick should the busy wait loop be done

			uint64_t currentTicks = desc->hpet->GetCounterTicks(desc->hpetDevice);
			uint64_t frequency = desc->hpet->GetCounterFrequency(desc->hpetDevice);

			uint64_t ticksToWait = (static_cast<uint128_t>(nanoseconds) * frequency) / NS_PER_SECOND;
			uint64_t deadline = currentTicks + ticksToWait;

			// busy wait loop
			while(desc->hpet->GetCounterTicks(desc->hpetDevice) < deadline)
			{
				Pause();
			}

			// Counter value has surpassed the deadline, return
			return;
		}
	}
	else
	{
		// Use the HPET Timer to interrupt once the wait is done

		sleepDone = false;
		hpetEvent = SleepEvent;
		if(!desc->hpet->ArmTimer(desc->hpetDevice, &hpetTimer, nanoseconds)) return;
		while(!sleepDone) SuspendCurrentCore();

		// SleepEvent has executed, return
		return;
	}
}

static void SleepEvent(ISR_InterruptStackFrame*)
{
	sleepDone = true;
}