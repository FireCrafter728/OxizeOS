const uint16_t GDT_64BIT_RING0_CODESEG = 0x08;
const uint16_t GDT_64BIT_RING0_DATASEG = 0x10;
const uint16_t GDT_64BIT_RING3_CODESEG = 0x18;
const uint16_t GDT_64BIT_RING3_DATASEG = 0x20;

const uint16_t GDT_32BIT_RING0_CODESEG = 0x28;
const uint16_t GDT_32BIT_RING0_DATASEG = 0x30;
const uint16_t GDT_32BIT_RING3_CODESEG = 0x38;
const uint16_t GDT_32BIT_RING3_DATASEG = 0x40;


TskSchl::MMD::MMD* TskSchl::mmd;
TskSchl::Paging::Paging* TskSchl::paging;

extern "C" void main(SystemTable* System)
{
	TskSchl::GDT::GDT gdt;

	TskSchl::GDT::GDT_Entry entries[] = {	
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

		// Ring 0 32-bit code segment entry, offset 0x28
		GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE | GDT_ACCESS_RING0 | GDT_ACCESS_PRESENT, GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

		// Ring 0 32-bit data segment entry, offset 0x30
		GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE | GDT_ACCESS_RING0 | GDT_ACCESS_PRESENT, GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

		// Ring 3 32-bit code segment entry, offset 0x38
		GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE | GDT_ACCESS_RING3 | GDT_ACCESS_PRESENT, GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

		// Ring 3 32-bit data segment entry, offset 0x40
		GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE | GDT_ACCESS_RING3 | GDT_ACCESS_PRESENT, GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),
	};

	gdt.Initialize(entries, sizeof(entries) / sizeof(entries[0]), GDT_64BIT_RING0_CODESEG, GDT_64BIT_RING0_DATASEG);

	TskSchl::IDT::IDT idt;
	idt.Initialize();

	TskSchl::ISR::ISR isr;
	isr.Initialize();

	TskSchl::Paging::Paging paging(System);

	TskSchl::MMD::MMIO mmio;
	if(!mmio.Initialize(TskSchl::MapAddr + System->memLayout.KrnlMemRegionSize, 0xFFFFFFFFFFFFFFFF - TskSchl::MapAddr - System->memLayout.KrnlMemRegionSize)) {
		printf("[TSKSCHL] [ERROR]: Failed to initialize MMIO Allocator\r\n");
		HaltSystem();
	}

	TskSchl::MMD::KRNL krnl;
	if(!krnl.Initialize(System->memLayout.DataAreaAddr)) {
		printf("[TSKSCHL] [ERROR]: Failed to initialize KRNL Allocator\r\n");
		HaltSystem();
	}

	TskSchl::MMD::MMD mmd;
	if(!mmd.Initialize(&mmio, &krnl)) {
		printf("[TSKSCHL] [ERROR]: Failed to initialize MMD\r\n");
		HaltSystem();
	}

	TskSchl::mmd = &mmd;
	TskSchl::paging = &paging;

	size_t framebufferSize = System->fb.currentResolution.resHeight * System->fb.currentResolution.resPitch;
	uint8_t* framebuffer = reinterpret_cast<uint8_t*>(mmd.malloc(BLOCK_COUNT(framebufferSize), TskSchl::MMD::MT_MMIO));

	if(!framebuffer) {
		printf("[TSKSCHL] [ERROR]: Failed to allocate memory for framebuffer\r\n");
		HaltSystem();
	}

	printf("framebuffer size: 0x%llX, framebuffer virt: 0x%llX, phys: 0x%llX, pageCount: 0x%llX\r\n", framebufferSize, framebuffer, System->fb.fbBase, BLOCK_COUNT(framebufferSize));

	paging.MapArea(System->fb.fbBase, reinterpret_cast<uintptr_t>(framebuffer), BLOCK_COUNT(framebufferSize), PTE_PRESENT | PTE_RW | PTE_CD | PTE_NX);

	for(size_t y = 0; y < System->fb.currentResolution.resHeight; y++)
	{
		uint32_t* row = reinterpret_cast<uint32_t*>(framebuffer + y * System->fb.currentResolution.resPitch);
		for(size_t x = 0; x < System->fb.currentResolution.resWidth; x++)
			row[x] = 0xFF0000CC;
	}

	TskSchl::ACPI::ACPI acpi;
	if(!acpi.Initialize(System)) {
		printf("[TSKSCHL] [ERROR]: Failed to initialize ACPI\r\n");
		HaltSystem();
	}

	TskSchl::ACPI::MCFG* mcfg = acpi.GetMCFG();
	if(!mcfg) {
		printf("[TSKSCHL] [ERROR]: Failed to get MCFG\r\n");
		HaltSystem();
	}

	TskSchl::PCIe::PCIe pcie;
	if(!pcie.Initialize(mcfg)) {
		printf("[TSKSCHL] [ERROR]: Failed to initialize PCIe\r\n");
		HaltSystem();
	}

	TskSchl::PCIe::DeviceInfo filters = {}, ahciPcieDevice;
	filters.VendorID = filters.DeviceID = PCIE_ANY16;
	filters.progIF = 0x01;
	filters.subClass = 0x06;
	filters.classCode = 0x01;

	if(!pcie.LocateDevice(&filters, &ahciPcieDevice)) {
		printf("[TSKSCHL] [ERROR]: Failed to locate SATA IntelAHCI Device\r\n");
		HaltSystem();
	}

	TskSchl::AHCI::AHCI ahci;
	TskSchl::AHCI::AHCIDevice ahciDevice;

	ahciDevice.deviceInfo = &ahciPcieDevice;

	if(!ahci.Initialize(&ahciDevice)) {
		printf("[TSKSCHL] [ERROR]: Failed to initialize SATA IntelAHCI Device\r\n");
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

	printf("[TSKSCHL] [INFO]: Found a device at port 0 of AHCI Controller with model name %s, serial number %s and firmware revision %s\r\n", AHCIDeviceModelNumber, AHCIDeviceSerialNumber, AHCIDeviceFirmwareRevision);

	// Test reading sectors 0 & 1 from DISK

	TskSchl::AHCI::AHCIDiskDevice diskDevice = {};
	diskDevice.controller = &ahciDevice;
	diskDevice.devicePort = 0;

	uint8_t sectorBuffer[2 * SECTOR_SIZE];

	if(!ahci.ReadSectors(&diskDevice, 0, 2, sectorBuffer)) {
		printf("[TSKSCHL] [ERROR]: Failed to read from DISK\r\n");
		HaltSystem();
	}

	for(size_t i = 0; i < sizeof(sectorBuffer); i++) printf("<0x%X> ", sectorBuffer[i]);
	
	puts("\r\n");

	HaltSystem();
}
