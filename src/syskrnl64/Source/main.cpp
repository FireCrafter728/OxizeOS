const uint16_t GDT_64BIT_RING0_CODESEG = 0x08;
const uint16_t GDT_64BIT_RING0_DATASEG = 0x10;
const uint16_t GDT_64BIT_RING3_CODESEG = 0x18;
const uint16_t GDT_64BIT_RING3_DATASEG = 0x20;

const uint16_t GDT_32BIT_RING0_CODESEG = 0x28;
const uint16_t GDT_32BIT_RING0_DATASEG = 0x30;
const uint16_t GDT_32BIT_RING3_CODESEG = 0x38;
const uint16_t GDT_32BIT_RING3_DATASEG = 0x40;


SysKrnl64::MMD::MMD* SysKrnl64::mmd;
SysKrnl64::MMD::MMD* SysKrnl64::MMD::mmd;
SysKrnl64::Paging::Paging* SysKrnl64::paging;
SysKrnl64::MSR::MSR* SysKrnl64::msr;

extern "C" void main(SystemTable* System)
{
	// Disable Interrupts

	DisableInterrupts();

	// Setup MSRs

	SysKrnl64::MSR::MSR msr;
	if(!msr.Initialize()) {
		printf("[SYSKRNL64] [ERROR]: CPU Doesn't support Model Specific Registers\r\n");
		HaltSystem();
	}
	SysKrnl64::msr = &msr;

	// Setup Global Description Table

	SysKrnl64::GDT::GDT gdt;

	SysKrnl64::GDT::GDT_Entry entries[] = {	
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

	gdt.Initialize(entries, sizeof(entries) / sizeof(entries[0]), GDT_64BIT_RING0_CODESEG, GDT_64BIT_RING0_DATASEG);

	// Setup IDT & ISRs

	SysKrnl64::IDT::IDT idt;
	idt.Initialize();

	SysKrnl64::ISR::ISR isr;
	isr.Initialize();

	// Setup paging & MMD drivers

	SysKrnl64::Paging::Paging paging(System);

	SysKrnl64::MMD::MMIO mmio;
	if(!mmio.Initialize(SysKrnl64::MapAddr + System->memLayout.KrnlMemRegionSize, 0xFFFFFFFFFFFFFFFF - SysKrnl64::MapAddr - System->memLayout.KrnlMemRegionSize)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize MMIO Allocator\r\n");
		HaltSystem();
	}

	SysKrnl64::MMD::KRNL krnl;
	if(!krnl.Initialize(System->memLayout.DataAreaAddr)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize KRNL Allocator\r\n");
		HaltSystem();
	}

	SysKrnl64::MMD::MMD mmd;
	if(!mmd.Initialize(&mmio, &krnl)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize MMD\r\n");
		HaltSystem();
	}

	SysKrnl64::MMD::mmd = &mmd;
	SysKrnl64::mmd = &mmd;
	SysKrnl64::paging = &paging;

	// Clear the screen to a blue color

	size_t framebufferSize = System->fb.currentResolution.resHeight * System->fb.currentResolution.resPitch;
	uint8_t* framebuffer = reinterpret_cast<uint8_t*>(mmd.malloc(BLOCK_COUNT(framebufferSize), SysKrnl64::MMD::MT_MMIO));

	if(!framebuffer) {
		printf("[SYSKRNL64] [ERROR]: Failed to allocate memory for framebuffer\r\n");
		HaltSystem();
	}

	paging.MapArea(System->fb.fbBase, reinterpret_cast<uintptr_t>(framebuffer), BLOCK_COUNT(framebufferSize), PTE_PRESENT | PTE_RW | PTE_CD | PTE_NX);

	for(size_t y = 0; y < System->fb.currentResolution.resHeight; y++)
	{
		uint32_t* row = reinterpret_cast<uint32_t*>(framebuffer + y * System->fb.currentResolution.resPitch);
		for(size_t x = 0; x < System->fb.currentResolution.resWidth; x++)
			row[x] = 0xFF0000CC;
	}

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

	SysKrnl64::ACPI::MADT* madt = acpi.GetMADT();
	if(!madt) {
		printf("[SYSKRNL64] [ERROR]: Failed to get MADT\r\n");
		HaltSystem();
	}

	// Setup APIC

	SysKrnl64::APIC::APIC apic;
	if(!apic.Initialize(madt)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize APIC\r\n");
		HaltSystem();
	}

	SysKrnl64::IRQ::IRQ irq;
	if(!irq.Initialize(&apic, &isr)) {
		printf("[SYSKRNL64] [ERROR]: Failed to initialize IRQ\r\n");
		HaltSystem();
	}

	SysKrnl64::IntHandlers::IntHandlers ih;
	if(!ih.Initialize(&isr, &irq)) {
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

	if(!ahci.ReadSectors(&diskDevice, 0, 2, sectorBuffer)) {
		printf("[SYSKRNL64] [ERROR]: Failed to read from DISK\r\n");
		HaltSystem();
	}

	for(size_t i = 0; i < sizeof(sectorBuffer); i++) printf("<0x%X> ", sectorBuffer[i]);
	
	puts("\r\n");

	EnableInterrupts();

	for(size_t i = 0; i < 10000000; i++);

	while(1);

	// HaltSystem();
}
