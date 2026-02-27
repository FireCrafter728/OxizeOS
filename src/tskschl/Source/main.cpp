const uint16_t GDT_64BIT_RING0_CODESEG = 0x08;
const uint16_t GDT_64BIT_RING0_DATASEG = 0x10;
const uint16_t GDT_64BIT_RING3_CODESEG = 0x18;
const uint16_t GDT_64BIT_RING3_DATASEG = 0x20;

const uint16_t GDT_32BIT_RING0_CODESEG = 0x28;
const uint16_t GDT_32BIT_RING0_DATASEG = 0x30;
const uint16_t GDT_32BIT_RING3_CODESEG = 0x38;
const uint16_t GDT_32BIT_RING3_DATASEG = 0x40;

class Test
{
public:
	Test() { val = 0xAA55; }
	uint32_t val;
};

Test test;

extern "C" void main(SystemTable* System)
{
	printf("test val: 0x%llX\r\n", test.val);

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

	paging.MapArea(System->fb.fbBase, System->memLayout.KrnlMemRegionSize + TskSchl::MapAddr, (System->fb.currentResolution.resPitch * System->fb.currentResolution.resHeight + TskSchl::PAGE_SIZE - 1) / TskSchl::PAGE_SIZE, TskSchl::Paging::PTE_PRESENT | TskSchl::Paging::PTE_RW | TskSchl::Paging::PTE_CD);

	uint32_t color = 0xFF0000FF;
	uint32_t* fb = reinterpret_cast<uint32_t*>(System->memLayout.KrnlMemRegionSize + TskSchl::MapAddr);

	for(size_t y = 0; y < System->fb.currentResolution.resHeight; y++)
	{
		uint8_t* row = (uint8_t*)fb + y * System->fb.currentResolution.resPitch;
		uint32_t* row32 = (uint32_t*)row;
		for(size_t x = 0; x < System->fb.currentResolution.resWidth; x++) row32[x] = color;
	}
	
	HaltSystem();
}
