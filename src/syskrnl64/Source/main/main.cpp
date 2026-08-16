// SPDX-License-Identifier: GPL-3.0-or-later

#include <main/defs.hpp>
#include <main/utils.hpp>

#include <arch/x86_64/Utility/io.hpp>
#include <arch/x86_64/Utility/itsc.hpp>
#include <arch/x86_64/Utility/cpuid.hpp>
#include <arch/x86_64/Utility/msr.hpp>

#include <arch/x86_64/MP/lpdata.hpp>

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

#include <stdio.hpp>
#include <string.hpp>
#include <string>
#include <stdint.h>
#include <stddef.h>

const uint16_t GDT_64BIT_RING0_CODESEG = 0x08;
const uint16_t GDT_64BIT_RING0_DATASEG = 0x10;
const uint16_t GDT_64BIT_RING3_CODESEG = 0x18;
const uint16_t GDT_64BIT_RING3_DATASEG = 0x20;

const uint16_t GDT_32BIT_RING0_CODESEG = 0x28;
const uint16_t GDT_32BIT_RING0_DATASEG = 0x30;
const uint16_t GDT_32BIT_RING3_CODESEG = 0x38;
const uint16_t GDT_32BIT_RING3_DATASEG = 0x40;

// Global extern pointers to drivers

krnl::Paging* krnl::paging;
krnl::MSR* krnl::msr;

krnl::PhysAlloc* krnl::physAlloc;
krnl::VirtAlloc* krnl::virtAlloc;
krnl::HeapAlloc* krnl::heapAlloc;

krnl::GDT_Entry* krnl::gdtEntries;

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

	krnl::MemoryAllocErrors res = s_physAlloc.Initialize(System);
	if(res != krnl::MMD_SUCCESS)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize Physical allocator, error code: %d\r\n", res);
		HaltSystem();
	}
	krnl::physAlloc = &s_physAlloc;

	// Setup virtual red-black tree allocator

	krnl::VA_VirtAllocDesc virtAllocDesc = {};
	virtAllocDesc.physAlloc = &s_physAlloc;
	virtAllocDesc.System = System;
	res = s_virtAlloc.Initialize(&virtAllocDesc);
	if(res != krnl::MMD_SUCCESS)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize Virtual allocator, error code: %d\r\n", res);
		HaltSystem();
	}
	krnl::virtAlloc = &s_virtAlloc;

	// Setup kernel heap allocator

	krnl::Heap_HeapAllocDesc heapAllocDesc = {};
	heapAllocDesc.physAlloc = &s_physAlloc;
	heapAllocDesc.virtAlloc = &s_virtAlloc;
	res = s_heapAlloc.Initialize(&heapAllocDesc);
	if(res != krnl::MMD_SUCCESS)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize Kernel heap allocator, error code: %d\r\n", res);
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

	krnl::gdtEntries = s_gdtEntries;
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

	// Reserve virtual memory for a new stack for the kernel

	auto kernelStackAllocRes = s_virtAlloc.AllocateBlocks(BLOCK_COUNT(KERNEL_STACK_SIZE) + 1, krnl::VA_NODE_FLAG_PHYSICALLY_NOT_BACKED | krnl::VA_NODE_FLAG_USED);
	if(!kernelStackAllocRes)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to reserve virtual memory for a new kernel stack with a size of 0x%llX, error: %d\r\n", KERNEL_STACK_SIZE, kernelStackAllocRes.error());
		HaltSystem();
	}

	// Allocate physical memory for the new kernel stack

	uint32_t kernelStackPhysAllocRes = s_physAlloc.AllocSparseBlocksToContiguousVirtualRange(BLOCK_COUNT(KERNEL_STACK_SIZE), reinterpret_cast<uintptr_t>(kernelStackAllocRes.value()) + BLOCK_SIZE, PTE_PRESENT | PTE_RW); // Skip mapping first block to make it a guard page to catch stack overflows
	if(kernelStackPhysAllocRes != krnl::MMD_SUCCESS)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to allocate physical memory for a new kernel stack, error: %d\r\n", kernelStackPhysAllocRes);
		HaltSystem();
	}

	return reinterpret_cast<uintptr_t>(kernelStackAllocRes.value()) + KERNEL_STACK_SIZE + BLOCK_SIZE;
}

extern "C" void kernel_main(SystemTable* System, uintptr_t StackAddr)
{
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

	API::ResourceMgr resourceMgr;
	API_STATUS apiStatus = resourceMgr.Initialize(System);
	if(API_ERROR(apiStatus))
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize resource manager, error code: 0x%lX\r\n", apiStatus);
		HaltSystem();
	}

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

	printf("[SYSKRNL64] [INFO]: OxizeOS x86-64 Kernel version %s build %s. Copyright (C) %s-%s OxizeOS authors, contributors\r\n", krnlVersion.c_str(), krnlBuild.c_str(), krnlCRDateStart.c_str(), krnlCRDateEnd.c_str());
	printf("[SYSKRNL64] [INFO]: Project licensed under %s\r\n", krnlLicense.c_str());
	printf("-------------------------------------------------------------------------------------------------------------------------\r\n");

	// Log kernel load addresses and load size
	printf("[SYSKRNL64] [INFO]: SysKrnl64 Physical load address: 0x%llX, SysKrnl64 Virtual load address: 0x%llX, SysKrnl64 load size: 0x%llX\r\n", System->memLayout.SysKrnl64PhysAddr, s_paging.GetKrnlStructVirt(System->memLayout.SysKrnl64PhysAddr), System->memLayout.SysKrnl64LoadSize);

	printf("[SYSKRNL64] [INFO]: GOP Framebuffer width: %llu, height: %llu, mapped at: 0x%llX\r\n", System->fb.currentResolution.resWidth, System->fb.currentResolution.resHeight, reinterpret_cast<uintptr_t>(framebuffer));

	// ----------------------------------------------------------------- //
	// Setup BSP TSS and assign different stacks for some CPU exceptions //
	// ----------------------------------------------------------------- //

	s_tss.rsp0 = StackAddr;
	
	s_tss.ioMapBase = sizeof(krnl::TSS);

	// IST1: Stack for the Double Fault(#DF) exception, vector 8, 16KiB stack

	auto ist1StackAllocRes = s_virtAlloc.AllocateBlocks(BLOCK_COUNT(IST1_STACK_SIZE) + 1, krnl::VA_NODE_FLAG_PHYSICALLY_NOT_BACKED | krnl::VA_NODE_FLAG_USED);
	if(!ist1StackAllocRes)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to reserve virtual memory for the BSP IST1 stack for the Double Fault exception, error: %d\r\n", ist1StackAllocRes.error());
		HaltSystem();
	}

	uint32_t ist1StackPhysAllocRes = s_physAlloc.AllocSparseBlocksToContiguousVirtualRange(BLOCK_COUNT(IST1_STACK_SIZE), reinterpret_cast<uintptr_t>(ist1StackAllocRes.value()) + BLOCK_SIZE, PTE_PRESENT | PTE_RW);
	if(ist1StackPhysAllocRes != krnl::MMD_SUCCESS)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to allocate physical memory for the BSP IST1 stack for the Double Fault exception, error: %d\r\n", ist1StackPhysAllocRes);
		HaltSystem();
	}

	s_tss.ist1 = reinterpret_cast<uintptr_t>(ist1StackAllocRes.value()) + IST1_STACK_SIZE + BLOCK_SIZE;

	// IST2: Stack for the Non-Maskable Interrupt exception, vector 2, 16KiB stack

	auto ist2StackAllocRes = s_virtAlloc.AllocateBlocks(BLOCK_COUNT(IST2_STACK_SIZE) + 1, krnl::VA_NODE_FLAG_PHYSICALLY_NOT_BACKED | krnl::VA_NODE_FLAG_USED);
	if(!ist2StackAllocRes)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to reserve virtual memory for the BSP IST2 stack for the Non-Maskable Interrupt exception, error: %d\r\n", ist2StackAllocRes.error());
		HaltSystem();
	}

	uint32_t ist2StackPhysAllocRes = s_physAlloc.AllocSparseBlocksToContiguousVirtualRange(BLOCK_COUNT(IST2_STACK_SIZE), reinterpret_cast<uintptr_t>(ist2StackAllocRes.value()) + BLOCK_SIZE, PTE_PRESENT | PTE_RW);
	if(ist2StackPhysAllocRes != krnl::MMD_SUCCESS)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to allocate physical memory for the BSP IST2 stack for the Non-Maskable Interrupt exception, error: %d\r\n", ist2StackPhysAllocRes);
		HaltSystem();
	}

	s_tss.ist2 = reinterpret_cast<uintptr_t>(ist2StackAllocRes.value()) + IST2_STACK_SIZE + BLOCK_SIZE;

	// IST3: Stack for the Machine Check exception, vector 18, 16KiB stack

	auto ist3StackAllocRes = s_virtAlloc.AllocateBlocks(BLOCK_COUNT(IST3_STACK_SIZE) + 1, krnl::VA_NODE_FLAG_PHYSICALLY_NOT_BACKED | krnl::VA_NODE_FLAG_USED);
	if(!ist3StackAllocRes)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to reserve virtual memory for the BSP IST3 stack for the Machine Check exception, error: %d\r\n", ist3StackAllocRes.error());
		HaltSystem();
	}

	uint32_t ist3StackPhysAllocRes = s_physAlloc.AllocSparseBlocksToContiguousVirtualRange(BLOCK_COUNT(IST3_STACK_SIZE), reinterpret_cast<uintptr_t>(ist3StackAllocRes.value()) + BLOCK_SIZE, PTE_PRESENT | PTE_RW);
	if(ist3StackPhysAllocRes != krnl::MMD_SUCCESS)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to allocate physical memory for the BSP IST3 stack for the Machine Check exception, error: %d\r\n", ist3StackPhysAllocRes);
		HaltSystem();
	}

	s_tss.ist3 = reinterpret_cast<uintptr_t>(ist3StackAllocRes.value()) + IST3_STACK_SIZE + BLOCK_SIZE;

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
	if(!apic.Initialize(madt)) {
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
	apiStatus = time.Initialize(&timer);
	if(API_ERROR(apiStatus))
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize time formatter, status: 0x%lX\r\n", apiStatus);
		HaltSystem();
	}

	// Log System Time

	API::TimeDate timeDate = time.GetTimeDate();

	printf("[SYSKRNL64] [INFO]: Current System Time(UTC): YYYY-MM-DD HH:MM:SS.NS %u-%u-%u %u:%u:%u.%llu\r\n", timeDate.date.year, timeDate.date.month, timeDate.date.day, timeDate.time.hour, timeDate.time.minute, timeDate.time.second, timeDate.time.nanosecond);

	HaltSystem();

	// ---------- //
	// Setup AHCI //
	// ---------- //

	krnl::PCIe_DeviceInfo filters = {}, ahciPcieDevice;
	filters.VendorID = filters.DeviceID = krnl::PCIE_ANY16;
	filters.progIF = 0x01;
	filters.subClass = 0x06;
	filters.classCode = 0x01;

	if(!pcie.LocateDevice(&filters, &ahciPcieDevice)) {
		printf("[SYSKRNL64] [ERROR]: Failed to locate SATA IntelAHCI Device\r\n");
		HaltSystem();
	}

	krnl::AHCI ahci;
	krnl::AHCIDevice ahciDevice;

	ahciDevice.deviceInfo = &ahciPcieDevice;

	if(!ahci.Initialize(&ahciDevice)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize SATA IntelAHCI Device\r\n");
		HaltSystem();
	}

	EnableInterrupts();

	HaltSystem();
}