// SPDX-License-Identifier: GPL-3.0-or-later

#include <main/defs.hpp>
#include <main/utils.hpp>

#include <arch/x86_64/Utility/io.hpp>
#include <arch/x86_64/Utility/itsc.hpp>
#include <arch/x86_64/Utility/cpuid.hpp>
#include <arch/x86_64/Utility/msr.hpp>
#include <arch/x86_64/Utility/alloc.hpp>

#include <arch/x86_64/MP/lpdata.hpp>
#include <arch/x86_64/MP/lcpu.hpp>

#include <arch/x86_64/Interrupts/gdt.hpp>
#include <arch/x86_64/Interrupts/idt.hpp>
#include <arch/x86_64/Interrupts/isr.hpp>
#include <arch/x86_64/Interrupts/tss.hpp>
#include <arch/x86_64/Interrupts/irq.hpp>
#include <arch/x86_64/Interrupts/handlers.hpp>

#include <arch/x86_64/PCI/PCIe.hpp>
#include <arch/x86_64/PCI/ahci.hpp>

#include <arch/x86_64/ACPI/apic.hpp>
#include <arch/x86_64/ACPI/acpi.hpp>
#include <arch/x86_64/ACPI/hpet.hpp>
#include <arch/x86_64/ACPI/timer.hpp>

#include <API/ResourceMgr/ResourceMgr.hpp>
#include <API/Time/time.hpp>
#include <API/HandleMgr/handle.hpp>

#include <stdio.hpp>
#include <string.hpp>
#include <stdint.h>
#include <stddef.h>

#include <queue>
#include <string>

const uint16_t GDT_64BIT_RING0_CODESEG = 0x08;
const uint16_t GDT_64BIT_RING0_DATASEG = 0x10;
const uint16_t GDT_64BIT_RING3_CODESEG = 0x18;
const uint16_t GDT_64BIT_RING3_DATASEG = 0x20;
const uint16_t GDT_TSS_DESC_OFFSET = 0x28;

// Global extern pointers to drivers

krnl::Paging* krnl::paging;
krnl::MSR* krnl::msr;

krnl::PhysAlloc* krnl::physAlloc;
krnl::VirtAlloc* krnl::virtAlloc;
krnl::HeapAlloc* krnl::heapAlloc;

// Static class defs to survive the stack switch

static krnl::Paging s_paging;
static krnl::MSR s_msr;

static krnl::PhysAlloc s_physAlloc;
static krnl::VirtAlloc s_virtAlloc;
static krnl::HeapAlloc s_heapAlloc;

static krnl::LPData s_lpData;
static krnl::LPSpecificData s_bspData;

static krnl::GDT s_gdt;
static krnl::IDT s_idt;
static krnl::ISR s_isr;
static krnl::TSS s_tss;

static krnl::GDT_Entry s_gdtEntries[TOTAL_GDT_ENTRIES] = {	
	// NULL Entry, offset 0x00
	GDT_ENTRY(0, 0, 0, 0),

	// Ring 0 64-bit code segment entry, offset 0x08
	GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE | GDT_ACCESS_RING0 | GDT_ACCESS_PRESENT, GDT_FLAG_64BIT | GDT_FLAG_GRANULARITY_4K),

	// Ring 0 64-bit data segment entry, offset 0x10
	GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE | GDT_ACCESS_RING0 | GDT_ACCESS_PRESENT, GDT_FLAG_64BIT | GDT_FLAG_GRANULARITY_4K),

	// Ring 3 64-bit code segment entry, offset 0x18
	GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE | GDT_ACCESS_RING3 | GDT_ACCESS_PRESENT, GDT_FLAG_64BIT | GDT_FLAG_GRANULARITY_4K),

	// Ring 3 64-bit data segment entry, offset 0x20
	GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE | GDT_ACCESS_RING3 | GDT_ACCESS_PRESENT, GDT_FLAG_64BIT | GDT_FLAG_GRANULARITY_4K),

	// Other slots reserved for TSS Descriptors
};

KRNL_STATUS InitializeTaskScheduler(krnl::LPID lpId, SystemTable* System, void* returnBuffer, uint64_t Parameter1, uint64_t Parameter2);

// kernel bootstrap function, sets up the kernel to it's final expected state and returns the pointer to the end of the new stack to switch to
extern "C" uint64_t kernel_bootstrap(SystemTable* System)
{
	// Disable Interrupts
	
	DisableInterrupts();

	// ---------- //
	// Setup MSRs //
	// ---------- //
	
	if(!s_msr.Initialize()) {
		printf("[SYSKRNL64] [ERROR]: CPU Doesn't support Model Specific Registers\r\n");
		HaltSystem();
	}
	krnl::msr = &s_msr;

	// -------------------------- //
	// Setup paging & MMD drivers //
	// -------------------------- //

	s_paging.Initialize(System);
	krnl::paging = &s_paging;

	// ----------------------- //
	// Setup memory allocators //
	// ----------------------- //

	// Setup physical bitmap allocator

	KRNL_STATUS res = s_physAlloc.Initialize(System);
	if(KRNL_ERROR(res))
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize Physical allocator, error code: %lu\r\n", res);
		HaltSystem();
	}
	krnl::physAlloc = &s_physAlloc;

	// Setup virtual red-black tree allocator

	krnl::VA_VirtAllocDesc virtAllocDesc = {};
	virtAllocDesc.physAlloc = &s_physAlloc;
	virtAllocDesc.System = System;
	res = s_virtAlloc.Initialize(&virtAllocDesc);
	if(KRNL_ERROR(res))
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize Virtual allocator, error code: %lu\r\n", res);
		HaltSystem();
	}
	krnl::virtAlloc = &s_virtAlloc;

	// Setup kernel heap allocator

	krnl::Heap_HeapAllocDesc heapAllocDesc = {};
	heapAllocDesc.physAlloc = &s_physAlloc;
	heapAllocDesc.virtAlloc = &s_virtAlloc;
	res = s_heapAlloc.Initialize(&heapAllocDesc);
	if(KRNL_ERROR(res))
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize Kernel heap allocator, error code: %lu\r\n", res);
		HaltSystem();
	}
	krnl::heapAlloc = &s_heapAlloc;

	// ----------------------- //
	// Setup BSP Specific data //
	// ----------------------- //

	if(!s_lpData.InitializeBSP(&s_bspData))
	{
		printf("[SYSKRNL64] [ERROR]: Failed to setup BSP Specific Data\r\n");
		HaltSystem();
	}

	// ------------------------------ //
	// Setup Global Description Table //
	// ------------------------------ //

	s_gdt.Initialize(s_gdtEntries, sizeof(s_gdtEntries) / sizeof(s_gdtEntries[0]), GDT_64BIT_RING0_CODESEG, GDT_64BIT_RING0_DATASEG);

	// ---------------- //
	// Setup IDT & ISRs //
	// ---------------- //

	s_idt.Initialize();	
	s_isr.Initialize();

	// ----------------------------- //
	// Setup BSP Task Switch Segment //
	// ----------------------------- //

	krnl::TSSDesc* tssDesc = reinterpret_cast<krnl::TSSDesc*>(s_gdtEntries + 5);
	*tssDesc = CONSTRUCT_TSS(reinterpret_cast<uintptr_t>(&s_tss), sizeof(krnl::TSS) - 1, krnl::TSS_PRESENT | krnl::TSS_TYPE_AVAILABLE);
	memset(&s_tss, 0, sizeof(krnl::TSS));
	LoadTSS(BSP_TASK_SWITCH_SEGMENT_OFFSET);

	// ------------------------------- //
	// Setup a new 512KiB kernel stack //
	// ------------------------------- //

	uintptr_t stackAddr = krnl::AllocateStack(KERNEL_STACK_SIZE, "new kernel stack");
	if(!stackAddr) HaltSystem();

	return stackAddr;
}

extern "C" void kernel_main(SystemTable* System, uintptr_t StackAddr)
{
	// --------------------- //
	// Initialize Handle Mgr //
	// --------------------- //

	API::HandleMgr::Initialize();

	// Clear the screen to a blue color

	size_t framebufferSize = System->fb.currentResolution.resHeight * System->fb.currentResolution.resPitch;
	auto fbAllocRes = s_virtAlloc.AllocateBlocks(BLOCK_COUNT(framebufferSize), krnl::VA_NODE_FLAG_MMIO | krnl::VA_NODE_FLAG_NO_EXECUTE_ACCESS | krnl::VA_NODE_FLAG_USED);
	if(!fbAllocRes)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to allocate memory for the GOP Framebuffer\r\n");
		HaltSystem();
	}
	s_paging.MapArea(System->fb.fbBase, reinterpret_cast<uintptr_t>(fbAllocRes.value()), BLOCK_COUNT(framebufferSize), PTE_PRESENT | PTE_RW | PTE_PCD | PTE_NX);
	uint8_t* framebuffer = reinterpret_cast<uint8_t*>(fbAllocRes.value());

	for(size_t y = 0; y < System->fb.currentResolution.resHeight; y++)
	{
		uint32_t* row = reinterpret_cast<uint32_t*>(framebuffer + y * System->fb.currentResolution.resPitch);
		for(size_t x = 0; x < System->fb.currentResolution.resWidth; x++)
			row[x] = 0xFF0000CC;
	}

	// ------------------------------- //
	// Initialize Resource Manager API //
	// ------------------------------- //

	API::ResourceMgr resourceMgr;
	KRNL_STATUS status = resourceMgr.Initialize(System);
	if(KRNL_ERROR(status))
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize resource manager, error code: 0x%lX\r\n", status);
		HaltSystem();
	}

	// Get kernel version strings

	std::string krnlVersion, krnlBuild, krnlLicense, krnlCRDateStart, krnlCRDateEnd;

	auto rsrcRes = resourceMgr.GetResourceByName("KERNEL_VERSION", API::RMgr_ResTypes::EXT_STRING);
	if(!rsrcRes)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to get kernel resource \"KERNEL_VERSION\"\r\n");
		HaltSystem();
	}
	krnlVersion = std::string(reinterpret_cast<const char*>(rsrcRes.value()->dataPtr), rsrcRes.value()->dataSize);

	rsrcRes = resourceMgr.GetResourceByName("KERNEL_BUILD", API::RMgr_ResTypes::EXT_STRING);
	if(!rsrcRes)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to get kernel resource \"KERNEL_BUILD\"\r\n");
		HaltSystem();
	}
	krnlBuild = std::string(reinterpret_cast<const char*>(rsrcRes.value()->dataPtr), rsrcRes.value()->dataSize);

	rsrcRes = resourceMgr.GetResourceByName("KERNEL_LICENSE", API::RMgr_ResTypes::EXT_STRING);
	if(!rsrcRes)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to get resource \"KERNEL_LICENSE\"\r\n");
		HaltSystem();
	}
	krnlLicense = std::string(reinterpret_cast<const char*>(rsrcRes.value()->dataPtr), rsrcRes.value()->dataSize);

	rsrcRes = resourceMgr.GetResourceByName("KERNEL_COPYRIGHT_START_DATE", API::RMgr_ResTypes::EXT_STRING);
	if(!rsrcRes)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to get resource \"KERNEL_COPYRIGHT_START_DATE\"\r\n");
		HaltSystem();
	}
	krnlCRDateStart = std::string(reinterpret_cast<const char*>(rsrcRes.value()->dataPtr), rsrcRes.value()->dataSize);

	rsrcRes = resourceMgr.GetResourceByName("KERNEL_COPYRIGHT_END_DATE", API::RMgr_ResTypes::EXT_STRING);
	if(!rsrcRes)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to get resource \"KERNEL_COPYRIGHT_END_DATE\"\r\n");
		HaltSystem();
	}
	krnlCRDateEnd = std::string(reinterpret_cast<const char*>(rsrcRes.value()->dataPtr), rsrcRes.value()->dataSize);

	// Log the start of the logfile

	printf("[SYSKRNL64] [INFO]: OxizeOS x86-64 Kernel version %s build %s. Copyright (C) %s-%s OxizeOS authors, contributors\r\n", krnlVersion.c_str(), krnlBuild.c_str(), krnlCRDateStart.c_str(), krnlCRDateEnd.c_str());
	printf("[SYSKRNL64] [INFO]: Project licensed under %s\r\n", krnlLicense.c_str());
	printf("-------------------------------------------------------------------------------------------------------------------------\r\n");

	// Log kernel load addresses and load size

	printf("[SYSKRNL64] [INFO]: SysKrnl64 Physical load address: 0x%llX, SysKrnl64 Virtual load address: 0x%llX, SysKrnl64 load size: 0x%llX\r\n", System->memLayout.SysKrnl64PhysAddr, s_paging.GetKrnlStructVirt(System->memLayout.SysKrnl64PhysAddr), System->memLayout.SysKrnl64LoadSize);

	// Log GOP Framebuffer info

	printf("[SYSKRNL64] [INFO]: GOP Framebuffer width: %llu, height: %llu, mapped at: 0x%llX\r\n", System->fb.currentResolution.resWidth, System->fb.currentResolution.resHeight, reinterpret_cast<uintptr_t>(framebuffer));

	// Log new kernel stack address

	printf("[SYSKRNL64] [INFO]: Kernel stack addr: 0x%llX, stack size: 0x%llX\r\n", StackAddr, KERNEL_STACK_SIZE);

	// ----------------------------------------------------------------- //
	// Setup BSP TSS and assign different stacks for some CPU exceptions //
	// ----------------------------------------------------------------- //

	s_tss.rsp0 = StackAddr;
	
	s_tss.ioMapBase = sizeof(krnl::TSS);

	// IST1: Stack for the Double Fault(#DF) exception, vector 8, 16KiB stack

	uintptr_t ist1StackAddr = krnl::AllocateStack(IST1_STACK_SIZE, "BSP IST1 stack for the Double Fault exception");
	if(!ist1StackAddr) HaltSystem();

	s_tss.ist1 = ist1StackAddr;

	// IST2: Stack for the Non-Maskable Interrupt exception, vector 2, 16KiB stack

	uintptr_t ist2StackAddr = krnl::AllocateStack(IST2_STACK_SIZE, "BSP IST2 stack for the Non Maskable Interrupt exception");
	if(!ist2StackAddr) HaltSystem();

	s_tss.ist2 = ist2StackAddr;

	// IST3: Stack for the Machine Check exception, vector 18, 16KiB stack

	uintptr_t ist3StackAddr = krnl::AllocateStack(IST3_STACK_SIZE, "BSP IST3 stack for the Machine Check exception");
	if(!ist3StackAddr) HaltSystem();

	s_tss.ist3 = ist3StackAddr;

	// ---------- //
	// Setup ACPI //
	// ---------- //

	krnl::ACPI acpi;
	if(!acpi.Initialize(System)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize ACPI\r\n");
		HaltSystem();
	}

	krnl::ACPI_MCFG* mcfg = acpi.GetMCFG();
	if(!mcfg) {
		printf("[SYSKRNL64] [ERROR]: Failed to get MCFG\r\n");
		HaltSystem();
	}

	printf("[SYSKRNL64] [INFO]: Located MCFG and mapped at address 0x%llX\r\n", reinterpret_cast<uintptr_t>(mcfg));

	krnl::ACPI_MADT* madt = acpi.GetMADT();
	if(!madt) {
		printf("[SYSKRNL64] [ERROR]: Failed to get MADT\r\n");
		HaltSystem();
	}

	printf("[SYSKRNL64] [INFO]: Located MADT and mapped at address 0x%llX\r\n", reinterpret_cast<uintptr_t>(madt));

	// ---------- //
	// Setup APIC //
	// ---------- //

	krnl::APIC apic;
	status = apic.Initialize(madt);
	if(KRNL_ERROR(status)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize APIC\r\n");
		HaltSystem();
	}

	krnl::IRQ irq;
	if(!irq.Initialize(&apic, &s_isr)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize IRQ\r\n");
		HaltSystem();
	}

	krnl::IntHandlers ih;
	if(!ih.Initialize(&s_isr, &irq)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize Interrupt Handlers\r\n");
		HaltSystem();
	}

	// ---------------------------------- //
	// Finish setting up LP Specific data //
	// ---------------------------------- //
	
	if(!s_lpData.Initialize(&apic, &s_bspData))
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize LP Specific data\r\n");
		HaltSystem();
	}

	// ---------- //
	// Setup PCIe //
	// ---------- //

	krnl::PCIe pcie;
	if(!pcie.Initialize(mcfg)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize PCIe\r\n");
		HaltSystem();
	}

	// ---------- //
	// Setup HPET //
	// ---------- //

	krnl::ACPI_HPET* acpiHpet = acpi.GetHPET();
	if(!acpiHpet)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to get ACPI HPET\r\n");
		HaltSystem();
	}
	
	printf("[SYSKRNL64] [INFO]: Located HPET and mapped at address 0x%llX\r\n", reinterpret_cast<uintptr_t>(acpiHpet));

	krnl::HPET_Timer hpet;
	krnl::HPET_Device hpetDevice = {};	
	hpetDevice = {};
	hpetDevice.apic = &apic;
	hpetDevice.hpet = acpiHpet;
	if(!hpet.Initialize(&hpetDevice))
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize HPET\r\n");
		HaltSystem();
	}

	// Enable interrupts, as inits after this point might require to use driver interrupts
	EnableInterrupts();

	// ---------- //
	// Setup iTSC //
	// ---------- //

	krnl::iTSC::Initialize();

	// ----------- //
	// Setup Timer //
	// ----------- //

	krnl::Timer timer;
	krnl::TimerDesc timerDesc = {};
	timerDesc.hpet = &hpet;
	timerDesc.hpetDevice = &hpetDevice;
	timerDesc.irq = &irq;
	timerDesc.System = System;
	if(!timer.Initialize(&timerDesc))
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize Timer\r\n");
		HaltSystem();
	}

	// ---------- //
	// Setup Time //
	// ---------- //

	API::Time time;
	status = time.Initialize(&timer);
	if(KRNL_ERROR(status))
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize time formatter, status: 0x%lX\r\n", status);
		HaltSystem();
	}

	// Log System Time

	API::TimeDate timeDate = time.GetTimeDate();

	printf("[SYSKRNL64] [INFO]: Current System Time(UTC): YYYY-MM-DD HH:MM:SS.NS %u-%u-%u %u:%u:%u.%lu\r\n", timeDate.date.year, timeDate.date.month, timeDate.date.day, timeDate.time.hour, timeDate.time.minute, timeDate.time.second, timeDate.time.nanosecond);

	// ---------- //
	// Setup LCPU //
	// ---------- //

	krnl::LCPU lcpu;
	krnl::LCPU_InitDesc lcpuInitDesc = {};
	lcpuInitDesc.apic = &apic;
	lcpuInitDesc.timer = &timer;
	lcpuInitDesc.lpData = &s_lpData;
	lcpuInitDesc.System = System;
	lcpuInitDesc.idt = &s_idt;
	lcpuInitDesc.isr = &s_isr;

	status = lcpu.Initialize(&lcpuInitDesc);
	if(KRNL_ERROR(status))
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize LCPU\r\n");
		HaltSystem();
	}

	// Setup the BSP Initial event

	uint64_t initialEventData[] = {
		reinterpret_cast<uint64_t>(InitializeTaskScheduler),
		0xAAAAAAAA55555555,
		0x123456789ABCDEF0,
	};

	krnl::LPEventData initialEventInfo = {};
	initialEventInfo.eventType = krnl::LP_EVENT_TYPE_EXECUTE;
	initialEventInfo.targetLPId = 0x00; // Doesn't matter
	initialEventInfo.eventData = initialEventData;
	initialEventInfo.eventDataLength = sizeof(initialEventData);
	
	auto ieCreateRes = lcpu.CreateEvent(&initialEventInfo);
	if(!ieCreateRes)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to create an initial BSP Event, error: 0x%lX\r\n", ieCreateRes.error());
		HaltSystem();
	}
	status = lcpu.EnterEventHandler(ieCreateRes.value(), System);
	printf("[SYSKRNL64] [ERROR]: Failed to enter the BSP Event handler, error: 0x%lX\r\n", status);
	HaltSystem();

	// End of kernel_main, continuation is in InitializeTaskScheduler

	// // ---------- //
	// // Setup AHCI //
	// // ---------- //

	// krnl::PCIe_DeviceInfo filters = {}, ahciPcieDevice;
	// filters.VendorID = filters.DeviceID = krnl::PCIE_ANY16;
	// filters.progIF = 0x01;
	// filters.subClass = 0x06;
	// filters.classCode = 0x01;

	// if(!pcie.LocateDevice(&filters, &ahciPcieDevice)) {
	// 	printf("[SYSKRNL64] [ERROR]: Failed to locate SATA IntelAHCI Device\r\n");
	// 	HaltSystem();
	// }

	// krnl::AHCI ahci;
	// krnl::AHCIDevice ahciDevice;

	// ahciDevice.deviceInfo = &ahciPcieDevice;

	// if(!ahci.Initialize(&ahciDevice)) {
	// 	printf("[SYSKRNL64] [ERROR]: Failed to initialize SATA IntelAHCI Device\r\n");
	// 	HaltSystem();
	// }

	// EnableInterrupts();

	// HaltSystem();
}

KRNL_STATUS InitializeTaskScheduler(krnl::LPID lpId, SystemTable* System, void* returnBuffer, uint64_t parameter1, uint64_t parameter2)
{
	printf("[SYSKRNL64] [INFO]: InitializeTaskScheduler was called with lpId 0x%lX, SystemTable ptr 0x%p, return buffer ptr 0x%p, parameter 1 0x%llX, parameter 2 0x%llX\r\n", lpId, System, returnBuffer, parameter1, parameter2);
	HaltSystem();
}