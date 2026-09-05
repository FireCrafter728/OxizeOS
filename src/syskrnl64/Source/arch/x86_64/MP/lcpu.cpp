// SPDX-License-Identifier: GPL-3.0-or-later

#include <main/utils.hpp>

#include <arch/x86_64/MP/lcpu.hpp>

#include <arch/x86_64/Utility/io.hpp>
#include <arch/x86_64/Utility/alloc.hpp>

#include <stdio.hpp>
#include <string.hpp>

#include <const_array.hpp>
#include <queue>
#include <mutex>

using namespace krnl;

ASMCALL uint8_t LPStartupFuncStart[];
ASMCALL uint8_t LPStartupFuncEnd[];
ASMCALL uint8_t LPStartupHeadersInstance[];

ASMCALL void LPCXXHandler(SystemTable* System, LPID lpId, LPInitComponents* initComponents, uint16_t* statusCode);
void LPWakeupInterruptHandler(ISR_InterruptStackFrame*);
void LP_ExecuteEvent(LPEventData* eventData, LPID lpId, SystemTable* System);
void LP_InvalidatePage(LPEventData* eventData, LPID lpId);
void LPEventHandler(SystemTable* System, LPID lpId);
static APIC* wakeupHandlerAPIC;

struct LPEventQueue
{
	std::mutex queueMutex;
	std::queue<LPEvent> queue;
};

stdEx::const_array<LPEventQueue> lpEvents;

KRNL_STATUS LCPU::Initialize(LCPU_InitDesc* initDesc)
{
	if(!initDesc || !initDesc->apic || !initDesc->timer || !initDesc->lpData || !initDesc->System || !initDesc->idt || !initDesc->isr)
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Invalid Initialize() input parameters\r\n");
		return KRNL_INVALID_PARAMETER;
	}

	this->initDesc = initDesc;
	wakeupHandlerAPIC = initDesc->apic;

	// Initialize the LP Descriptors by using the LPSpecificData const array and the APIC CPUThreadDescs array

	stdEx::const_array<LPSpecificData> lpSpecificData = initDesc->lpData->GetLPDataArray();
	std::vector<APIC_CPUThreadDesc> cpuThreadDescs = initDesc->apic->getCPUThreads();

	lpDescs.init(lpSpecificData.size());

	for(size_t i = 0; i < lpSpecificData.size(); i++)
	{
		LPDesc* desc = &lpDescs[i];
		LPSpecificData* data = &lpSpecificData[i];
		APIC_CPUThreadDesc* threadDesc = &cpuThreadDescs[i];

		desc->identity = data->identity;
		desc->lpSpecificDataPtr = data->self;

		// Determine the LP state from the LAPIC / X2APIC flags
		// If flags bits 0 and 1 are both cleared, the LP is fully disabled and cannot be brought online
		// If the LP is the BSP, it's already online

		if(threadDesc->bsp) 
		{
			desc->lpState = LP_STATE_ONLINE;
			bspId = desc->identity.lpid;
		}
		else if((threadDesc->flags & 0x01) == 0 && (threadDesc->flags & 0x02) == 0) desc->lpState = LP_STATE_FIRMWARE_DISABLED;
		else desc->lpState = LP_STATE_UNINITIALIZED;
	}

	// Initialize lpEvents vector
	lpEvents.init(lpDescs.size());

	// Copy the trampoline code to the SMP Bringup page

	memcpy(reinterpret_cast<void*>(initDesc->System->memLayout.SMPThreadBringupPageAddr), LPStartupFuncStart, reinterpret_cast<uintptr_t>(LPStartupFuncEnd) - reinterpret_cast<uintptr_t>(LPStartupFuncStart));

	printf("[SYSKRNL64] [LCPU] [INFO]: Successfully copied the LP Bringup trampoline to address 0x%llX\r\n", initDesc->System->memLayout.SMPThreadBringupPageAddr);

	// Allocate an event type page
	this->eventTypePage = API::HandleMgr::RegisterTypeRange();

	// Initialize each LP

	KRNL_STATUS status = KRNL_SUCCESS;

	for(size_t i = 0; i < lpDescs.size(); i++)
	{
		LPDesc* desc = &lpDescs[i];
		if(desc->lpState == LP_STATE_ONLINE) continue; // Skip the BSP

		LPInitComponents initComponents = {}; // Keep init components alive until the LP has finished initializing

		status = SetupLPInitContext(desc, &initComponents);
		if(KRNL_ERROR(status)) return status;

		LPStartupHeader* lpStartupHeader = reinterpret_cast<LPStartupHeader*>(initDesc->System->memLayout.SMPThreadBringupPageAddr + (reinterpret_cast<uintptr_t>(LPStartupHeadersInstance) & (PAGE_SIZE - 1)));

		// Setup LP idle event

		LPEventData idleEventData = {};
		idleEventData.eventType = LP_EVENT_TYPE_IDLE;
		idleEventData.targetLPId = desc->identity.lpid;
		auto idleEventRes = CreateEvent(&idleEventData);
		if(!idleEventRes)
		{
			printf("[SYSKRNL64] [LCPU] [ERROR]: Failed to create an idle event for LP %lu, status: 0x%lX\r\n", desc->identity.lpid, idleEventRes.error());
			HaltSystem();
		}
		lpEvents[desc->identity.lpid].queue.push(idleEventRes.value());

		desc->lpState = LP_STATE_INITIALIZING;

		// Send an INIT IPI to the Processor local APIC
		SendINIT_IPI(desc);

		// Send the Startup IPIs
		SendStartupIPI(desc);

		// Poll the Status until it's set to LP_STARTUP_STATUS_BOOTSTRAP_COMPLETE
		while(lpStartupHeader->statusCode == LP_STARTUP_STATUS_INITIALIZING) Pause();

		if(lpStartupHeader->statusCode != LP_STARTUP_STATUS_BOOTSTRAP_COMPLETE)
		{
			printf("[SYSKRNL64] [LCPU] [ERROR]: LP %lu startup failed with error code 0x%X\r\n", desc->identity.lpid, lpStartupHeader->statusCode);
			return KRNL_LCPU_LP_STARTUP_FAILED;
		}

		// Set the status to LP_STARTUP_STATUS_EXECUTE_CXX_HANDLER
		lpStartupHeader->statusCode = LP_STARTUP_STATUS_EXECUTE_CXX_HANDLER;

		// Poll the status until it changes from what we set previously
		while(lpStartupHeader->statusCode == LP_STARTUP_STATUS_EXECUTE_CXX_HANDLER) Pause();

		// Check the new status
		if(lpStartupHeader->statusCode != LP_INIT_SUCCEEDED)
		{
			printf("[SYSKRNL64] [LCPU] [ERROR]: LP %lu initialization failed with error code 0x%X\r\n", desc->identity.lpid, lpStartupHeader->statusCode);
			return KRNL_LCPU_LP_INIT_FAILED;
		}

		// LP Initialized successfully, set it's state to online
		desc->lpState = LP_STATE_ONLINE;

		printf("[SYSKRNL64] [LCPU] [INFO]: Successfully initialized LP %lu\r\n", desc->identity.lpid);
	}

	return KRNL_SUCCESS;
}

KRNL_STATUS LCPU::SetupLPInitContext(LPDesc* lpDesc, LPInitComponents* initComponents)
{
	// Fill in the LP Startup header

	LPStartupHeader* lpStartupHeader = reinterpret_cast<LPStartupHeader*>(initDesc->System->memLayout.SMPThreadBringupPageAddr + (reinterpret_cast<uintptr_t>(LPStartupHeadersInstance) & (PAGE_SIZE - 1)));

	// Set the stack segment to be the same as the LPStartup page, and the offset to 0x1000 to point to the top of the stack
	// This allows the bringup page to be a single page before a segment split, because in real mode if sp / ip changes, ss / cs doesn't get updated
	lpStartupHeader->startupStackSegment = initDesc->System->memLayout.SMPThreadBringupPageAddr >> 4;
	lpStartupHeader->startupStackOffset = 0x1000;

	lpStartupHeader->lpId = lpDesc->identity.lpid;
	lpStartupHeader->CR3Value = GetCR3();
	lpStartupHeader->systemTablePtr = reinterpret_cast<uintptr_t>(initDesc->System);
	lpStartupHeader->cxxHandlerPtr = reinterpret_cast<uintptr_t>(LPCXXHandler);
	lpStartupHeader->statusCode = LP_STARTUP_STATUS_INITIALIZING;

	// Setup the initialization components struct needed to initialize the LP in C++

	initComponents->lpData = initDesc->lpData;

	// Allocate the GDT and GDT Entries in the heap to ensure their lifetime
	initComponents->gdt = reinterpret_cast<GDT*>(kmalloc(sizeof(GDT)));
	if(!initComponents->gdt)
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Failed to allocate memory for the GDT for initializing LP %lu\r\n", lpDesc->identity.lpid);
		return KRNL_MEMORY_ALLOC_FAILED;
	} 

	// Need 7 total GDT Entries: 
	// 1 NULL selector;
	// 1 ring 0 64-bit code;
	// 1 ring 0 64-bit data;
	// 1 ring 3 64-bit code;
	// 1 ring 3 64-bit data;
	// 2 for the TSS Descriptor

	initComponents->gdtEntries = reinterpret_cast<GDT_Entry*>(kmalloc(sizeof(GDT_Entry) * 7));
	if(!initComponents->gdtEntries)
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Failed to allocate memory for the GDT entry array for initializing LP %lu\r\n", lpDesc->identity.lpid);
		return KRNL_MEMORY_ALLOC_FAILED;
	}

	initComponents->gdtEntries[0] = GDT_ENTRY(0, 0, 0, 0); // NULL selector

	initComponents->gdtEntries[1] = GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE | GDT_ACCESS_RING0 | GDT_ACCESS_PRESENT, GDT_FLAG_64BIT | GDT_FLAG_GRANULARITY_4K); // Ring 0 64-bit code segment, offset 0x08

	initComponents->gdtEntries[2] = GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE | GDT_ACCESS_RING0 | GDT_ACCESS_PRESENT, GDT_FLAG_64BIT | GDT_FLAG_GRANULARITY_4K); // Ring 0 64-bit data segment, offset 0x10

	initComponents->gdtEntries[3] = GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE | GDT_ACCESS_RING3 | GDT_ACCESS_PRESENT, GDT_FLAG_64BIT | GDT_FLAG_GRANULARITY_4K); // Ring 3 64-bit code segment, offset 0x18

	initComponents->gdtEntries[4] = GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE | GDT_ACCESS_RING3 | GDT_ACCESS_PRESENT, GDT_FLAG_64BIT | GDT_FLAG_GRANULARITY_4K); // Ring 3 64-bit data segment, offset 0x20

	initComponents->gdtEntryCount = 7;

	initComponents->gdtCSSelector = GDT_64BIT_RING0_CODESEG;
	initComponents->gdtDSSelector = GDT_64BIT_RING0_DATASEG;

	// Store the IDT ptr

	initComponents->idt = initDesc->idt;
	initComponents->isr = initDesc->isr;

	// Setup the TSS

	initComponents->tssDescOffset = GDT_TSS_DESC_OFFSET;

	TSS* tss = reinterpret_cast<TSS*>(kmalloc(sizeof(TSS)));
	if(!tss)
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Failed to allocate memory for the TSS for initializing LP %lu\r\n", lpDesc->identity.lpid);
		return KRNL_MEMORY_ALLOC_FAILED;
	}
	tss->ioMapBase = sizeof(TSS);

	// Allocate a 4KiB ring 3 -> ring 0 transition
	tss->rsp0 = AllocateStack(LP_PRIVILEGE_SWITCH_STACK_SIZE, "LP privilege transition stack", "LCPU");
	if(!tss->rsp0) return KRNL_STACK_ALLOC_FAILED;

	// Allocate the IST1-IST3 stacks

	// IST1: Stack for the Double Fault(#DF) exception, vector 8, 16KiB stack

	uintptr_t ist1StackAddr = krnl::AllocateStack(IST1_STACK_SIZE, "LP IST1 stack for the Double Fault exception", "LCPU");
	if(!ist1StackAddr) return KRNL_MEMORY_ALLOC_FAILED;

	tss->ist1 = ist1StackAddr;

	// IST2: Stack for the Non-Maskable Interrupt exception, vector 2, 16KiB stack

	uintptr_t ist2StackAddr = krnl::AllocateStack(IST2_STACK_SIZE, "LP IST2 stack for the Non Maskable Interrupt exception", "LCPU");
	if(!ist2StackAddr) return KRNL_STACK_ALLOC_FAILED;

	tss->ist2 = ist2StackAddr;

	// IST3: Stack for the Machine Check exception, vector 18, 16KiB stack

	uintptr_t ist3StackAddr = krnl::AllocateStack(IST3_STACK_SIZE, "LP IST3 stack for the Machine Check exception", "LCPU");
	if(!ist3StackAddr) return KRNL_STACK_ALLOC_FAILED;

	tss->ist3 = ist3StackAddr;

	// Construct the TSS Descriptor

	TSSDesc* desc = reinterpret_cast<TSSDesc*>(reinterpret_cast<uintptr_t>(initComponents->gdtEntries) + GDT_TSS_DESC_OFFSET);
	*desc = CONSTRUCT_TSS(reinterpret_cast<uintptr_t>(tss), sizeof(TSS) - 1, TSS_PRESENT | TSS_TYPE_AVAILABLE);

	initComponents->tssDescOffset = GDT_TSS_DESC_OFFSET;

	// Store the APIC driver ptr

	initComponents->apic = initDesc->apic;

	// Store the init components

	lpStartupHeader->lpInitComponents = reinterpret_cast<uintptr_t>(initComponents);
	
	// Allocate a 32 KiB bootstrap stack 

	uintptr_t bootstrapStackAddr = krnl::AllocateStack(LP_BOOTSTRAP_STACK_SIZE, "LP Bootstrap stack", "LCPU");
	if(!bootstrapStackAddr) return KRNL_STACK_ALLOC_FAILED;

	lpStartupHeader->bootstrapStackTop = bootstrapStackAddr;

	// Apply the relocations in the LP Startup function
	// Relocations needed:
	// LPStartupHeadersInstance + sizeof(LPStartupHeaders) + 1: set to LPStartupHeaders & 0xF
	// LPStartupHeadersInstance + sizeof(LPStartupHeaders) + 4: set to LPStartupHeaders >> 4

	uintptr_t AfterHeadersAddr = (reinterpret_cast<uintptr_t>(LPStartupHeadersInstance) & (PAGE_SIZE - 1)) + sizeof(LPStartupHeader) + initDesc->System->memLayout.SMPThreadBringupPageAddr;

	uint16_t* relocValue = reinterpret_cast<uint16_t*>(AfterHeadersAddr + 1);
	*relocValue = reinterpret_cast<uintptr_t>(lpStartupHeader) & 0x0F;
	relocValue = reinterpret_cast<uint16_t*>(AfterHeadersAddr + 4);
	*relocValue = reinterpret_cast<uintptr_t>(lpStartupHeader) >> 4;

	return KRNL_SUCCESS;
}

KRNL_STATUS LCPU::EnterEventHandler(LPEvent initialEvent, SystemTable* System)
{
	auto eventDataRes = API::HandleMgr::GetHandleData(initialEvent);
	if(!eventDataRes)
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Failed to get BSPs initial event data, error: 0x%lX\r\n", eventDataRes.error());
		return KRNL_LCPU_INVALID_LP_EVENT;
	}
	
	LPEventData* eventData = reinterpret_cast<LPEventData*>(eventDataRes.value());

	if(eventData == nullptr)
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Invalid initial event data ptr for the BSP\r\n");
		return KRNL_LCPU_INVALID_LP_EVENT_DATA;
	}

	if(eventData->eventType != LP_EVENT_TYPE_EXECUTE)
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: BSP Initial event type isn't executable\r\n");
		return KRNL_LCPU_INVALID_LP_EVENT_DATA;
	}

	if(eventData->eventFlags != 0)
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Invalid BSP initial event flags\r\n");
		return KRNL_LCPU_INVALID_LP_EVENT_DATA;
	}

	lpEvents[bspId].queue.push(initialEvent);

	LPEventHandler(System, bspId);

	return KRNL_LCPU_LP_STARTUP_FAILED; // Should never happen
}

void LCPU::SendINIT_IPI(LPDesc* lpDesc)
{
	// Write the INIT IPI command to the LAPIC ICR register, with low value having delivery mode = INIT, level = assert and trigger mode = level

	initDesc->apic->WriteLapicICR(lpDesc->identity.apicId, LAPIC_ICR_DELIVERY_INIT | LAPIC_ICR_LEVEL_ASSERT | LAPIC_ICR_TRIGGER_LEVEL);

	// Wait until the LAPIC ICR Delivery status is cleared
	while(initDesc->apic->ReadLAPIC(LAPIC_ICR_LOW) & LAPIC_ICR_DELIVERY_STATUS) Pause();

	// Sleep 10ms before sending the first SIPI
	initDesc->timer->SleepMS(10);
}

void LCPU::SendStartupIPI(LPDesc* lpDesc)
{
	// Calculate the Startup trampoline vector from the address by shifting it right by 12 bits

	uint8_t startupVector = initDesc->System->memLayout.SMPThreadBringupPageAddr >> 12;

	// Send the first SIPI

	initDesc->apic->WriteLapicICR(lpDesc->identity.apicId, LAPIC_ICR_DELIVERY_SIPI | startupVector);

	// Wait until the delivery status is cleared

	while(initDesc->apic->ReadLAPIC(LAPIC_ICR_LOW) & LAPIC_ICR_DELIVERY_STATUS) Pause();

	// Sleep 200 microseconds before sending the second SIPI(200000 nanoseconds)

	initDesc->timer->SleepNS(200000);

	// send a second SIPI in case the LP missed the first SIPI

	initDesc->apic->WriteLapicICR(lpDesc->identity.apicId, LAPIC_ICR_DELIVERY_SIPI | startupVector);

	// Wait until the delivery status is cleared

	while(initDesc->apic->ReadLAPIC(LAPIC_ICR_LOW) & LAPIC_ICR_DELIVERY_STATUS) Pause();
}

void LCPU::SendWakeupIPI(LPDesc* lpDesc)
{
	initDesc->apic->WriteLapicICR(lpDesc->identity.apicId, LAPIC_ICR_DELIVERY_FIXED | ISR_LP_WAKEUP);

	while(initDesc->apic->ReadLAPIC(LAPIC_ICR_LOW) & LAPIC_ICR_DELIVERY_STATUS) Pause();
}

ASMCALL void LPCXXHandler(SystemTable* System, LPID lpId, LPInitComponents* initComponents, uint16_t* returnStatus)
{
	// Inititialize LP Specific data 

	if(!initComponents->lpData->InitializeLP(lpId))
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Failed to initialize LP Specific data for the LP %lu\r\n", lpId);
		*returnStatus = LP_SPECIFIC_DATA_INIT_FAILED;
		HaltSystem();
	}

	// Initialize GDT

	initComponents->gdt->Initialize(initComponents->gdtEntries, initComponents->gdtEntryCount, initComponents->gdtCSSelector, initComponents->gdtDSSelector);

	// Initialize IDT

	initComponents->idt->Initialize();

	// Register the Wakeup handler

	initComponents->isr->RegisterHandler(ISR_LP_WAKEUP, LPWakeupInterruptHandler);

	// Initialize TSS

	LoadTSS(initComponents->tssDescOffset);

	// Initialize Local APIC

	KRNL_STATUS status = initComponents->apic->InitializeCurrentLP();
	if(KRNL_ERROR(status))
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Failed to initialize LAPIC for LP %lu\r\n", lpId);
		*returnStatus = LP_LAPIC_INIT_FAILED;
		HaltSystem();
	}

	*returnStatus = LP_INIT_SUCCEEDED;

	LPEventHandler(System, lpId);
	HaltSystem();
}

void LPEventHandler(SystemTable* System, LPID lpId)
{
	while(true)
	{
		DisableInterrupts(); // Disable interrupts while checking the queue to ensure that an event wouldn't arrive after the queue.empty check and before the SuspendCurrentCore()
		while(true)
		{
			LPEvent event;

			{
				std::lock_guard<std::mutex> lock(lpEvents[lpId].queueMutex);

				if(lpEvents[lpId].queue.empty()) break;

				event = lpEvents[lpId].queue.front();
				lpEvents[lpId].queue.pop();
			} 

			EnableInterrupts();

			LPEventData* eventData = nullptr;

			auto eventDataRes = API::HandleMgr::GetHandleData(event);
			if(!eventDataRes)
			{
				printf("[SYSKRNL64] [LCPU] [ERROR]: Failed to get LP %lu event data, error: 0x%lX\r\n", lpId, eventDataRes.error());
				continue;
			}

			eventData = reinterpret_cast<LPEventData*>(eventDataRes.value());
			if(eventData == nullptr)
			{
				printf("[SYSKRNL64] [LCPU] [ERROR]: Invalid Event data ptr for the LP %lu\r\n", lpId);
				continue;
			}

			if(eventData->eventType == LP_EVENT_TYPE_UNKNOWN) printf("[SYSKRNL64] [LCPU] [WARN]: Received event with event type UNKNOWN on LP %lu\r\n", lpId);
			else if(eventData->eventType == LP_EVENT_TYPE_IDLE);
			else if(eventData->eventType == LP_EVENT_TYPE_EXECUTE) LP_ExecuteEvent(eventData, lpId, System);
			else if(eventData->eventType == LP_EVENT_TYPE_INVALIDATE_PAGE) LP_InvalidatePage(eventData, lpId);
			eventData->completed.store(true, std::memory_order_release);
		}
		SuspendCurrentCore();
	}
}

void LPWakeupInterruptHandler(ISR_InterruptStackFrame*)
{
	// Send LAPIC EOI since this is supposed to be called by an IPI, not a regular INT instruction
	wakeupHandlerAPIC->SendEOI();
}

void LP_ExecuteEvent(LPEventData* eventData, LPID lpId, SystemTable* System)
{
	eventData->handlerStatus = KRNL_SUCCESS;

	uint64_t* data = reinterpret_cast<uint64_t*>(eventData->eventData);
	if(!data)
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Invalid Event data ptr for the event type EXECUTE for the LP %lu\r\n", lpId);
		eventData->handlerStatus = KRNL_LCPU_INVALID_LP_EVENT_DATA;
		return;
	}
	
	// Check whether to execute a single function or multiple
	if(eventData->eventFlags & LP_EVENT_FLAG_MULTIENTRY)
	{
		size_t functionCount = *data++;

		if(functionCount == 0)
		{
			printf("[SYSKRNL64] [LCPU] [ERROR]: Event with type EXECUTE has no functions to execute for the LP %lu\r\n", lpId);
			eventData->handlerStatus = KRNL_LCPU_INVALID_LP_EVENT_DATA;
			return;
		}

		if(eventData->eventDataLength < sizeof(uint64_t) * functionCount * 3 + sizeof(uint64_t))
		{
			printf("[SYSKRNL64] [LCPU] [ERROR]: Event data length %llu for the event type EXECUTE with multifunction flag is below the needed length of %llu for the LP %lu\r\n", eventData->eventDataLength, sizeof(uint64_t) * functionCount * 3 + sizeof(uint64_t), lpId);
			eventData->handlerStatus = KRNL_LCPU_INVALID_LP_EVENT_DATA;
			return;
		}

		if(eventData->returnBufferElementLength == 0 && eventData->returnBufferLength != 0)
		{
			printf("[SYSKRNL64] [LCPU] [ERROR]: Event data return buffer element length is 0 while return buffer length isn't 0 for the event type EXECUTE with multifunction flag for the LP %lu\r\n", lpId);
			eventData->handlerStatus = KRNL_LCPU_INVALID_LP_EVENT_DATA;
			return;
		}

		if(eventData->returnBufferElementLength * functionCount > eventData->returnBufferLength)
		{
			printf("[SYSKRNL64] [LCPU] [ERROR]: Return buffer is too small to fit all of the function returns for the event type EXECUTE with multifunction flag for the LP %lu\r\n", lpId);
			eventData->handlerStatus = KRNL_LCPU_INVALID_LP_EVENT_DATA;
			return;
		}

		if(eventData->returnBuffer == nullptr && (eventData->returnBufferElementLength != 0 || eventData->returnBufferLength != 0))
		{
			printf("[SYSKRNL64] [LCPU] [ERROR]: Return buffer is NULL while it's described length / element length is not 0 for the event type EXECUTE with multifunction flag for the LP %lu\r\n", lpId);
			eventData->handlerStatus = KRNL_LCPU_INVALID_LP_EVENT_DATA;
			return;
		}

		uint8_t* returnBuffer = reinterpret_cast<uint8_t*>(eventData->returnBuffer);
		for(size_t i = 0; i < functionCount; i++)
		{
			uintptr_t functionPtr = *data++;
			uint64_t parameter1 = *data++;
			uint64_t parameter2 = *data++;

			if(functionPtr == 0)
			{
				printf("[SYSKRNL64] [LCPU] [ERROR]: Function %llu ptr is NULL for the event type EXECUTE with multifunction flag for the LP %lu\r\n", i, lpId);
				eventData->handlerStatus = KRNL_LCPU_INVALID_LP_EVENT_DATA;
				return;
			}

			LPEventFunction func = reinterpret_cast<LPEventFunction>(functionPtr);

			KRNL_STATUS status = func(lpId, System, reinterpret_cast<void*>(returnBuffer), parameter1, parameter2);
			returnBuffer += eventData->returnBufferElementLength;
			if(KRNL_ERROR(status))
			{
				printf("[SYSKRNL64] [LCPU] [ERROR]: Function %llu returned an error code 0x%lX for the event type EXECUTE with multifunction flag for the LP %lu\r\n", i, status, lpId);
				eventData->handlerStatus = status;
				return;
			}
		}
		return;
	}

	// Single function event

	if(eventData->eventDataLength < sizeof(uint64_t) * 3)
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Event data length %llu for the event type EXECUTE is below the minimum of %llu for the LP %lu\r\n", eventData->eventDataLength, sizeof(uint64_t) * 3, lpId);
		eventData->handlerStatus = KRNL_LCPU_INVALID_LP_EVENT_DATA;
		return;
	}

	uintptr_t functionPtr = *data++;
	uint64_t parameter1 = *data++;
	uint64_t parameter2 = *data++;

	if(functionPtr == 0)
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Function ptr is NULL for the event type EXECUTE for the LP %lu\r\n", lpId);
		eventData->handlerStatus = KRNL_LCPU_INVALID_LP_EVENT_DATA;
		return;
	}

	LPEventFunction func = reinterpret_cast<LPEventFunction>(functionPtr);

	eventData->handlerStatus = func(lpId, System, eventData->returnBuffer, parameter1, parameter2);
	if(KRNL_ERROR(eventData->handlerStatus))
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Function returned an error code 0x%lX for the event type EXECUTE for the LP %lu\r\n", eventData->handlerStatus, lpId);
		return;
	}

	return;
}

void LP_InvalidatePage(LPEventData* eventData, LPID lpId)
{
	eventData->handlerStatus = KRNL_SUCCESS;

	uint64_t* data = reinterpret_cast<uint64_t*>(eventData->eventData);
	if(!data)
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Invalid Event data ptr for the event type INVALIDATE PAGE for the LP %lu\r\n", lpId);
		eventData->handlerStatus = KRNL_LCPU_INVALID_LP_EVENT_DATA;
		return;
	}
	
	if(eventData->eventDataLength < sizeof(uint64_t))
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Event data length %llu for the event type INVALIDATE PAGE is below the needed length of %llu for the LP %lu\r\n", eventData->eventDataLength, sizeof(uint64_t), lpId);
		eventData->handlerStatus = KRNL_LCPU_INVALID_LP_EVENT_DATA;
		return;
	}

	// Check whether to execute a single function or multiple
	if(eventData->eventFlags & LP_EVENT_FLAG_MULTIENTRY)
	{
		uint64_t pageCount = *data++;
		if(pageCount == 0)
		{
			printf("[SYSKRNL64] [LCPU] [ERROR]: Event with type INVALIDATE PAGE with multifunction flag has no pages to invalidate for the LP %lu\r\n", lpId);
			eventData->handlerStatus = KRNL_LCPU_INVALID_LP_EVENT_DATA;
			return;
		}

		if(eventData->eventDataLength < sizeof(uint64_t) + sizeof(uint64_t) * pageCount)
		{
			printf("[SYSKRNL64] [LCPU] [ERROR]: Event data length %llu for the event type INVALIDATE PAGE with multifunction flag is below the needed length of %llu for the LP %lu", eventData->eventDataLength, sizeof(uint64_t) + sizeof(uint64_t) * pageCount, lpId);
			eventData->handlerStatus = KRNL_LCPU_INVALID_LP_EVENT_DATA;
			return;
		}

		for(size_t i = 0; i < pageCount; i++) InvalidatePage(*data++);

		return;
	}

	InvalidatePage(*data);
}

std::expected<LPEvent, KRNL_STATUS> LCPU::CreateEvent(LPEventData* eventData)
{
	if(!eventData)
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Invalid CreateEvent input parameters\r\n");
		return std::unexpected<KRNL_STATUS>(KRNL_INVALID_PARAMETER);
	}

	eventData->completed.store(false, std::memory_order_relaxed);

	return static_cast<LPEvent>(API::HandleMgr::CreateHandle(eventTypePage + LCPU_HANDLE_TYPE_EVENT, eventData, sizeof(LPEventData)));
}

KRNL_STATUS LCPU::ExecuteEvent(LPEvent event, bool waitUntilFinish)
{
	auto dataRes = API::HandleMgr::GetHandleData(static_cast<Handle>(event));
	if(!dataRes)
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Failed to get event data, error: 0x%lX\r\n", dataRes.error());
		return dataRes.error();
	}

	LPEventData* data = reinterpret_cast<LPEventData*>(dataRes.value());

	if(!data)
	{
		printf("[SYSKRNL64] [LCPU] [ERROR]: Invalid event data\r\n");
		return KRNL_LCPU_INVALID_LP_EVENT_DATA;
	}

	{
		std::lock_guard<std::mutex> lock(lpEvents[data->targetLPId].queueMutex);
		lpEvents[data->targetLPId].queue.push(event);	
	}

	SendWakeupIPI(&lpDescs[data->targetLPId]); // Signal the target LP that a new event has appeared

	if(waitUntilFinish)
	{
		while(!data->completed.load(std::memory_order_acquire)) Pause();
	}

	return data->handlerStatus;
}

KRNL_STATUS LCPU::DestroyEvent(LPEvent event)
{
	return API::HandleMgr::DestroyHandle(static_cast<Handle>(event));
}