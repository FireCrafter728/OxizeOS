#include <idt.h>
#include <util.h>

typedef struct {
    uint16_t BaseLow;
    uint16_t SegmentSelector;
    uint8_t Reserved;
    uint8_t Flags;
    uint16_t BaseHigh;
} __attribute__((packed)) IDTEntry;

typedef struct {
    uint16_t Limit;
    IDTEntry* Ptr;
} __attribute__((packed)) IDTDescriptor;

IDTEntry IDT[256];

IDTDescriptor IDTDesc = {
    sizeof(IDT) - 1,
    IDT
};

void __attribute__((cdecl)) IDT_Load(IDTDescriptor* IDTDesc);

void IDT_SetGate(uint8_t Interrupt, void* base, uint16_t segmentSelector, uint8_t flags)
{
    IDT[Interrupt].BaseLow = ((uint32_t)base) & 0xFFFF;
    IDT[Interrupt].SegmentSelector = segmentSelector;
    IDT[Interrupt].Reserved = 0;
    IDT[Interrupt].Flags = flags;
    IDT[Interrupt].BaseHigh = ((uint32_t)base >> 16) & 0xFFFF;
}

void IDT_EnableGate(uint8_t Interrupt)
{
    FLAG_SET(IDT[Interrupt].Flags, IDT_FLAG_PRESENT);
}

void IDT_DisableGate(uint8_t Interrupt)
{
    FLAG_UNSET(IDT[Interrupt].Flags, IDT_FLAG_PRESENT);
}

void IDT_Initialize()
{
    IDT_Load(&IDTDesc);
}