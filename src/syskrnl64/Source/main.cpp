// SPDX-License-Identifier: GPL-3.0-or-later

const uint16_t GDT_64BIT_RING0_CODESEG = 0x08;
const uint16_t GDT_64BIT_RING0_DATASEG = 0x10;
const uint16_t GDT_64BIT_RING3_CODESEG = 0x18;
const uint16_t GDT_64BIT_RING3_DATASEG = 0x20;

const uint16_t GDT_32BIT_RING0_CODESEG = 0x28;
const uint16_t GDT_32BIT_RING0_DATASEG = 0x30;
const uint16_t GDT_32BIT_RING3_CODESEG = 0x38;
const uint16_t GDT_32BIT_RING3_DATASEG = 0x40;

// Global extern pointers to drivers

SysKrnl64::Paging::Paging* SysKrnl64::paging;
SysKrnl64::MSR::MSR* SysKrnl64::msr;

SysKrnl64::MMD::PhysAlloc* SysKrnl64::physAlloc;
SysKrnl64::MMD::VirtAlloc* SysKrnl64::virtAlloc;
SysKrnl64::MMD::HeapAlloc* SysKrnl64::heapAlloc;

SysKrnl64::GDT::GDT_Entry* SysKrnl64::gdtEntries;

// Static class defs to survive the stack switch

static SysKrnl64::Paging::Paging s_paging;
static SysKrnl64::MSR::MSR s_msr;

static SysKrnl64::MMD::PhysAlloc s_physAlloc;
static SysKrnl64::MMD::VirtAlloc s_virtAlloc;
static SysKrnl64::MMD::HeapAlloc s_heapAlloc;

static SysKrnl64::GDT::GDT s_gdt;
static SysKrnl64::IDT::IDT s_idt;
static SysKrnl64::ISR::ISR s_isr;
static SysKrnl64::GDT::TSS s_tss;

static SysKrnl64::GDT::GDT_Entry s_gdtEntries[TOTAL_GDT_ENTRIES] = {	
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

	printf("[SYSKRNL64] [INFO]: OxizeOS x86-64 Kernel version 1.0.0 build 0012. Copyright (C) 2025-2026 OxizeOS authors, contributors\r\n");
	printf("[SYSKRNL64] [INFO]: Project licensed under GNU General Public License v3.0 or later\r\n");
	printf("-------------------------------------------------------------------------------------------------------------------------\r\n");

	// ---------- //
	// Setup MSRs //
	// ---------- //
	
	if(!s_msr.Initialize()) {
		printf("[SYSKRNL64] [ERROR]: CPU Doesn't support Model Specific Registers\r\n");
		HaltSystem();
	}
	SysKrnl64::msr = &s_msr;

	// ------------------------------ //
	// Setup Global Description Table //
	// ------------------------------ //

	SysKrnl64::gdtEntries = s_gdtEntries;
	s_gdt.Initialize(s_gdtEntries, sizeof(s_gdtEntries) / sizeof(s_gdtEntries[0]), GDT_64BIT_RING0_CODESEG, GDT_64BIT_RING0_DATASEG);

	// ---------------- //
	// Setup IDT & ISRs //
	// ---------------- //

	s_idt.Initialize();	
	s_isr.Initialize();

	// ----------------------------- //
	// Setup BSP Task Switch Segment //
	// ----------------------------- //

	SysKrnl64::GDT::TSSDesc* tssDesc = reinterpret_cast<SysKrnl64::GDT::TSSDesc*>(s_gdtEntries + 5);
	*tssDesc = CONSTRUCT_TSS(reinterpret_cast<uintptr_t>(&s_tss), sizeof(SysKrnl64::GDT::TSS) - 1, SysKrnl64::GDT::TSS_PRESENT | SysKrnl64::GDT::TSS_TYPE_AVAILABLE);
	memset(&s_tss, 0, sizeof(SysKrnl64::GDT::TSS));
	SysKrnl64::GDT::LoadTSS(BSP_TASK_SWITCH_SEGMENT_OFFSET);

	// -------------------------- //
	// Setup paging & MMD drivers //
	// -------------------------- //

	s_paging.Initialize(System);
	SysKrnl64::paging = &s_paging;

	// Log kernel load addresses and load size
	printf("[SYSKRNL64] [INFO]: SysKrnl64 Physical load address: 0x%llX, SysKrnl64 Virtual load address: 0x%llX, SysKrnl64 load size: 0x%llX\r\n", System->memLayout.SysKrnl64PhysAddr, s_paging.GetKrnlStructVirt(System->memLayout.SysKrnl64PhysAddr), System->memLayout.SysKrnl64LoadSize);

	// ----------------------- //
	// Setup memory allocators //
	// ----------------------- //

	// Setup physical bitmap allocator

	SysKrnl64::MMD::MemoryAllocErrors res = s_physAlloc.Initialize(System);
	if(res != SysKrnl64::MMD::MMD_SUCCESS)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize Physical allocator, error code: %d\r\n", res);
		HaltSystem();
	}
	SysKrnl64::physAlloc = &s_physAlloc;

	// Setup virtual red-black tree allocator

	SysKrnl64::MMD::VirtAllocDesc virtAllocDesc = {};
	virtAllocDesc.physAlloc = &s_physAlloc;
	virtAllocDesc.System = System;
	res = s_virtAlloc.Initialize(&virtAllocDesc);
	if(res != SysKrnl64::MMD::MMD_SUCCESS)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize Virtual allocator, error code: %d\r\n", res);
		HaltSystem();
	}
	SysKrnl64::virtAlloc = &s_virtAlloc;

	// Setup kernel heap allocator

	SysKrnl64::MMD::HeapAllocDesc heapAllocDesc = {};
	heapAllocDesc.physAlloc = &s_physAlloc;
	heapAllocDesc.virtAlloc = &s_virtAlloc;
	res = s_heapAlloc.Initialize(&heapAllocDesc);
	if(res != SysKrnl64::MMD::MMD_SUCCESS)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize Kernel heap allocator, error code: %d\r\n", res);
		HaltSystem();
	}
	SysKrnl64::heapAlloc = &s_heapAlloc;

	// ------------------------------- //
	// Setup a new 512KiB kernel stack //
	// ------------------------------- //

	// Reserve virtual memory for a new stack for the kernel

	auto kernelStackAllocRes = s_virtAlloc.AllocateBlocks(BLOCK_COUNT(KERNEL_STACK_SIZE) + 1, SysKrnl64::MMD::VA_NODE_FLAG_PHYSICALLY_NOT_BACKED | SysKrnl64::MMD::VA_NODE_FLAG_USED);
	if(!kernelStackAllocRes)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to reserve virtual memory for a new kernel stack with a size of 0x%llX, error: %d\r\n", KERNEL_STACK_SIZE, kernelStackAllocRes.error());
		HaltSystem();
	}

	// Allocate physical memory for the new kernel stack

	uint32_t kernelStackPhysAllocRes = s_physAlloc.AllocSparseBlocksToContiguousVirtualRange(BLOCK_COUNT(KERNEL_STACK_SIZE), reinterpret_cast<uintptr_t>(kernelStackAllocRes.value()) + BLOCK_SIZE, PTE_PRESENT | PTE_RW); // Skip mapping first block to make it a guard page to catch stack overflows
	if(kernelStackPhysAllocRes != SysKrnl64::MMD::MMD_SUCCESS)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to allocate physical memory for a new kernel stack, error: %d\r\n", kernelStackPhysAllocRes);
		HaltSystem();
	}

	printf("[SYSKRNL64] [INFO]: Successfully allocated a new kernel stack with size 0x%llX at address 0x%llX\r\n", KERNEL_STACK_SIZE, reinterpret_cast<uintptr_t>(kernelStackAllocRes.value()));

	return reinterpret_cast<uintptr_t>(kernelStackAllocRes.value()) + KERNEL_STACK_SIZE + BLOCK_SIZE;
}



extern "C" void kernel_main(SystemTable* System, uintptr_t StackAddr)
{
	// Clear the screen to a blue color

	size_t framebufferSize = System->fb.currentResolution.resHeight * System->fb.currentResolution.resPitch;
	auto fbAllocRes = s_virtAlloc.AllocateBlocks(BLOCK_COUNT(framebufferSize), SysKrnl64::MMD::VA_NODE_FLAG_MMIO | SysKrnl64::MMD::VA_NODE_FLAG_NO_EXECUTE_ACCESS | SysKrnl64::MMD::VA_NODE_FLAG_USED);
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

	printf("[SYSKRNL64] [INFO]: GOP Framebuffer width: %llu, height: %llu, mapped at: 0x%llX\r\n", System->fb.currentResolution.resWidth, System->fb.currentResolution.resHeight, reinterpret_cast<uintptr_t>(framebuffer));

	// ----------------------------------------------------------------- //
	// Setup BSP TSS and assign different stacks for some CPU exceptions //
	// ----------------------------------------------------------------- //

	s_tss.rsp0 = StackAddr;
	
	s_tss.ioMapBase = sizeof(SysKrnl64::GDT::TSS);

	// IST1: Stack for the Double Fault(#DF) exception, vector 8, 16KiB stack

	auto ist1StackAllocRes = s_virtAlloc.AllocateBlocks(BLOCK_COUNT(IST1_STACK_SIZE) + 1, SysKrnl64::MMD::VA_NODE_FLAG_PHYSICALLY_NOT_BACKED | SysKrnl64::MMD::VA_NODE_FLAG_USED);
	if(!ist1StackAllocRes)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to reserve virtual memory for the BSP IST1 stack for the Double Fault exception, error: %d\r\n", ist1StackAllocRes.error());
		HaltSystem();
	}

	uint32_t ist1StackPhysAllocRes = s_physAlloc.AllocSparseBlocksToContiguousVirtualRange(BLOCK_COUNT(IST1_STACK_SIZE), reinterpret_cast<uintptr_t>(ist1StackAllocRes.value()) + BLOCK_SIZE, PTE_PRESENT | PTE_RW);
	if(ist1StackPhysAllocRes != SysKrnl64::MMD::MMD_SUCCESS)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to allocate physical memory for the BSP IST1 stack for the Double Fault exception, error: %d\r\n", ist1StackPhysAllocRes);
		HaltSystem();
	}

	s_tss.ist1 = reinterpret_cast<uintptr_t>(ist1StackAllocRes.value()) + 5 * BLOCK_SIZE;

	// IST2: Stack for the Non-Maskable Interrupt exception, vector 2, 16KiB stack

	auto ist2StackAllocRes = s_virtAlloc.AllocateBlocks(BLOCK_COUNT(IST2_STACK_SIZE) + 1, SysKrnl64::MMD::VA_NODE_FLAG_PHYSICALLY_NOT_BACKED | SysKrnl64::MMD::VA_NODE_FLAG_USED);
	if(!ist2StackAllocRes)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to reserve virtual memory for the BSP IST2 stack for the Non-Maskable Interrupt exception, error: %d\r\n", ist2StackAllocRes.error());
		HaltSystem();
	}

	uint32_t ist2StackPhysAllocRes = s_physAlloc.AllocSparseBlocksToContiguousVirtualRange(BLOCK_COUNT(IST2_STACK_SIZE), reinterpret_cast<uintptr_t>(ist2StackAllocRes.value()) + BLOCK_SIZE, PTE_PRESENT | PTE_RW);
	if(ist2StackPhysAllocRes != SysKrnl64::MMD::MMD_SUCCESS)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to allocate physical memory for the BSP IST2 stack for the Non-Maskable Interrupt exception, error: %d\r\n", ist2StackPhysAllocRes);
		HaltSystem();
	}

	s_tss.ist2 = reinterpret_cast<uintptr_t>(ist2StackAllocRes.value()) + 5 * BLOCK_SIZE;

	// IST3: Stack for the Machine Check exception, vector 18, 16KiB stack

	auto ist3StackAllocRes = s_virtAlloc.AllocateBlocks(BLOCK_COUNT(IST3_STACK_SIZE) + 1, SysKrnl64::MMD::VA_NODE_FLAG_PHYSICALLY_NOT_BACKED | SysKrnl64::MMD::VA_NODE_FLAG_USED);
	if(!ist3StackAllocRes)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to reserve virtual memory for the BSP IST3 stack for the Machine Check exception, error: %d\r\n", ist3StackAllocRes.error());
		HaltSystem();
	}

	uint32_t ist3StackPhysAllocRes = s_physAlloc.AllocSparseBlocksToContiguousVirtualRange(BLOCK_COUNT(IST3_STACK_SIZE), reinterpret_cast<uintptr_t>(ist3StackAllocRes.value()) + BLOCK_SIZE, PTE_PRESENT | PTE_RW);
	if(ist3StackPhysAllocRes != SysKrnl64::MMD::MMD_SUCCESS)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to allocate physical memory for the BSP IST3 stack for the Machine Check exception, error: %d\r\n", ist3StackPhysAllocRes);
		HaltSystem();
	}

	s_tss.ist3 = reinterpret_cast<uintptr_t>(ist3StackAllocRes.value()) + 5 * BLOCK_SIZE;

	// ---------- //
	// Setup ACPI //
	// ---------- //

	SysKrnl64::ACPI::ACPI acpi;
	if(!acpi.Initialize(System)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize ACPI\r\n");
		HaltSystem();
	}

	SysKrnl64::ACPI::MCFG* mcfg = acpi.GetMCFG();
	if(!mcfg) {
		printf("[SYSKRNL64] [ERROR]: Failed to get MCFG\r\n");
		HaltSystem();
	}

	printf("[SYSKRNL64] [INFO]: Located MCFG and mapped at address 0x%llX\r\n", reinterpret_cast<uintptr_t>(mcfg));

	SysKrnl64::ACPI::MADT* madt = acpi.GetMADT();
	if(!madt) {
		printf("[SYSKRNL64] [ERROR]: Failed to get MADT\r\n");
		HaltSystem();
	}

	printf("[SYSKRNL64] [INFO]: Located MADT and mapped at address 0x%llX\r\n", reinterpret_cast<uintptr_t>(madt));

	// ---------- //
	// Setup APIC //
	// ---------- //

	SysKrnl64::APIC::APIC apic;
	if(!apic.Initialize(madt)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize APIC\r\n");
		HaltSystem();
	}

	SysKrnl64::IRQ::IRQ irq;
	if(!irq.Initialize(&apic, &s_isr)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize IRQ\r\n");
		HaltSystem();
	}

	SysKrnl64::IntHandlers::IntHandlers ih;
	if(!ih.Initialize(&s_isr, &irq)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize Interrupt Handlers\r\n");
		HaltSystem();
	}

	// ---------- //
	// Setup PCIe //
	// ---------- //

	SysKrnl64::PCIe::PCIe pcie;
	if(!pcie.Initialize(mcfg)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize PCIe\r\n");
		HaltSystem();
	}

	// ---------- //
	// Setup HPET //
	// ---------- //

	SysKrnl64::ACPI::HPET* acpiHpet = acpi.GetHPET();
	if(!acpiHpet)
	{
		printf("[SYSKRNL64] [ERROR]: Failed to get ACPI HPET\r\n");
		HaltSystem();
	}
	
	printf("[SYSKRNL64] [INFO]: Located HPET and mapped at address 0x%llX\r\n", reinterpret_cast<uintptr_t>(acpiHpet));

	SysKrnl64::Timer::HPET_Timer hpet;
	SysKrnl64::Timer::HPETDevice hpetDevice = {};	
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
	// Setup LCPU //
	// ---------- //

	SysKrnl64::MP::LCPU lcpu;
	if(!lcpu.Initialize(&apic))
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize LCPU\r\n");
		HaltSystem();
	}

	// ---------- //
	// Setup iTSC //
	// ---------- //

	if(!SysKrnl64::Timer::iTSC::Initialize())
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize iTSC\r\n");
		HaltSystem();
	}

	// ----------- //
	// Setup Timer //
	// ----------- //

	SysKrnl64::Timer::Timer timer;
	SysKrnl64::Timer::TimerDesc timerDesc = {};
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

	SysKrnl64::Timer::Time time;
	if(!time.Initialize(&timer))
	{
		printf("[SYSKRNL64] [ERROR]: Failed to initialize time formatter\r\n");
		HaltSystem();
	}

	// Log System Time

	SysKrnl64::Timer::TimeDate timeDate = time.GetTimeDate();

	printf("[SYSKRNL64] [INFO]: Current System Time(UTC): YYYY-MM-DD HH:MM:SS.NS %u-%u-%u %u:%u:%u.%llu\r\n", timeDate.date.year, timeDate.date.month, timeDate.date.day, timeDate.time.hour, timeDate.time.minute, timeDate.time.second, timeDate.time.nanosecond);

	HaltSystem();

	// ---------- //
	// Setup AHCI //
	// ---------- //

	SysKrnl64::PCIe::DeviceInfo filters = {}, ahciPcieDevice;
	filters.VendorID = filters.DeviceID = PCIE_ANY16;
	filters.progIF = 0x01;
	filters.subClass = 0x06;
	filters.classCode = 0x01;

	if(!pcie.LocateDevice(&filters, &ahciPcieDevice)) {
		printf("[SYSKRNL64] [ERROR]: Failed to locate SATA IntelAHCI Device\r\n");
		HaltSystem();
	}

	SysKrnl64::AHCI::AHCI ahci;
	SysKrnl64::AHCI::AHCIDevice ahciDevice;

	ahciDevice.deviceInfo = &ahciPcieDevice;

	if(!ahci.Initialize(&ahciDevice)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize SATA IntelAHCI Device\r\n");
		HaltSystem();
	}

	EnableInterrupts();

	HaltSystem();
}
