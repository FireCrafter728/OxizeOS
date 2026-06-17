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

// Static class defs to survive the stack switch

static SysKrnl64::Paging::Paging s_paging;
static SysKrnl64::MSR::MSR s_msr;

static SysKrnl64::MMD::PhysAlloc s_physAlloc;
static SysKrnl64::MMD::VirtAlloc s_virtAlloc;
static SysKrnl64::MMD::HeapAlloc s_heapAlloc;

static SysKrnl64::GDT::GDT s_gdt;
static SysKrnl64::IDT::IDT s_idt;
static SysKrnl64::ISR::ISR s_isr;

static SysKrnl64::GDT::GDT_Entry s_gdtEntries[] = {	
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
};

// kernel bootstrap function, sets up the kernel to it's final expected state and returns the pointer to the end of the new stack to switch to
extern "C" uint64_t kernel_bootstrap(SystemTable* System)
{
	// Disable Interrupts

	DisableInterrupts();

	printf("[SYSKRNL64] [INFO]: Initialized logfile\r\n");

	// Setup MSRs
	
	if(!s_msr.Initialize()) {
		printf("[SYSKRNL64] [ERROR]: CPU Doesn't support Model Specific Registers\r\n");
		HaltSystem();
	}
	SysKrnl64::msr = &s_msr;

	// Setup Global Description Table

	s_gdt.Initialize(s_gdtEntries, sizeof(s_gdtEntries) / sizeof(s_gdtEntries[0]), GDT_64BIT_RING0_CODESEG, GDT_64BIT_RING0_DATASEG);

	// Setup IDT & ISRs

	s_idt.Initialize();	
	s_isr.Initialize();

	// Setup paging & MMD drivers

	s_paging.Initialize(System);
	SysKrnl64::paging = &s_paging;

	// Log kernel load addresses and load size
	printf("[SYSKRNL64] [INFO]: SysKrnl64 Physical load address: 0x%llX, SysKrnl64 Virtual load address: 0x%llX, SysKrnl64 load size: 0x%llX\r\n", System->memLayout.SysKrnl64PhysAddr, s_paging.GetKrnlStructVirt(System->memLayout.SysKrnl64PhysAddr), System->memLayout.SysKrnl64LoadSize);

	// Setup memory allocators

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

extern "C" void kernel_main(SystemTable* System)
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

	// Setup ACPI

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

	// Setup APIC

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

	// Setup PCIe

	SysKrnl64::PCIe::PCIe pcie;
	if(!pcie.Initialize(mcfg)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize PCIe\r\n");
		HaltSystem();
	}

	// Setup AHCI

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

	char AHCIDeviceModelNumber[41]; // Words 27-46
	char AHCIDeviceSerialNumber[21]; // Words 10-19
	char AHCIDeviceFirmwareRevision[9]; // Words 23-26

	volatile uint16_t* bufferWords = reinterpret_cast<volatile uint16_t*>(ahciDevice.ports[0].IdentifyBuffer);

	auto extractStr = [&](char* out, uint16_t startWord, uint16_t endWord) {
		char* tmp = out;
		for(uint16_t word = startWord; word <= endWord; word++)
		{	
			uint16_t pair = bufferWords[word];
			*(tmp++) = (pair >> 8) & 0xFF;
			*(tmp++) = pair & 0xFF;
		}

		uint16_t chars = (endWord - startWord) * 2;

		for(int16_t i = chars - 1; i >= 0; i--)
		{
			if(out[i] == ' ') {
				out[i] = '\0';
				continue;
			}

			out[i + 1] = '\0';
			break;
		}
	};

	extractStr(AHCIDeviceModelNumber, 27, 46);
	extractStr(AHCIDeviceSerialNumber, 10, 19);
	extractStr(AHCIDeviceFirmwareRevision, 23, 26);

	printf("[SYSKRNL64] [INFO]: Found a device at port 0 of AHCI Controller with model name %s, serial number %s and firmware revision %s\r\n", AHCIDeviceModelNumber, AHCIDeviceSerialNumber, AHCIDeviceFirmwareRevision);

	// Test reading sectors 0 & 1 from DISK

	SysKrnl64::AHCI::AHCIDiskDevice diskDevice = {};
	diskDevice.controller = &ahciDevice;
	diskDevice.devicePort = 0;

	uint8_t sectorBuffer[2 * SECTOR_SIZE];
	printf("sectorBuffer virt: 0x%llX, phys: 0x%llX, phys2: 0x%llX\r\n", reinterpret_cast<uintptr_t>(&sectorBuffer), s_paging.GetPhys(reinterpret_cast<uintptr_t>(&sectorBuffer)), s_paging.GetPhys(reinterpret_cast<uintptr_t>(&sectorBuffer) + SECTOR_SIZE));

	if(!ahci.ReadSectors(&diskDevice, 0, 2, &sectorBuffer)) {
		printf("[SYSKRNL64] [ERROR]: Failed to read from DISK\r\n");
		HaltSystem();
	}

	printf("sectorBuffer virt: 0x%llX, phys: 0x%llX, phys2: 0x%llX\r\n", reinterpret_cast<uintptr_t>(&sectorBuffer), s_paging.GetPhys(reinterpret_cast<uintptr_t>(&sectorBuffer)), s_paging.GetPhys(reinterpret_cast<uintptr_t>(&sectorBuffer) + SECTOR_SIZE));

	for(size_t i = 0; i < sizeof(sectorBuffer); i++) printf("<0x%X> ", sectorBuffer[i]);
	
	puts("\r\n");

	EnableInterrupts();

	while(1);

	HaltSystem();
}
