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

	TskSchl::PCIe::DeviceInfo filters = {}, device;
	filters.VendorID = filters.DeviceID = PCIE_ANY16;
	filters.progIF = 0x01;
	filters.subClass = 0x06;
	filters.classCode = 0x01;

	if(!pcie.LocateDevice(&filters, &device)) {
		printf("[TSKSCHL] [ERROR]: Failed to locate SATA IntelAHCI Device\r\n");
		HaltSystem();
	}

	printf("SATA IntelAHCI Device properties:\r\n");
	printf("VendorID: 0x%X, DeviceID: 0x%X\r\n", device.VendorID, device.DeviceID);
	printf("bus: 0x%X, device: 0x%X\r\n\r\n", device.Bus, device.Device);
	for(size_t i = 0; i < 6; i++)
	{
		if(device.BARs[i].BarAddr == 0) continue; // Either non-existent BAR or a 64-bit extension
		printf("PCIe BAR Index %d: Type: %d, Arch: %d, Size: 0x%llX, Address: 0x%llX\r\n", i, device.BARs[i].BarType, device.BARs[i].BarArch, device.BARs[i].BarLength, device.BARs[i].BarAddr);
	}

	HaltSystem();
}
