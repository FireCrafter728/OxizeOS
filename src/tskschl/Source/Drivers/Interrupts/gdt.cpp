#include <Drivers/Interrupts/gdt.hpp>

using namespace TskSchl::GDT;

ASMCALL void GDT_Load(TskSchl::GDT::GDT_Desc* desc, uint16_t newCs, uint16_t newDs);

void GDT::Initialize(GDT_Entry* entries, size_t entryCount, uint16_t newCs, uint16_t newDs)
{
    gdtDesc.Limit = entryCount * sizeof(GDT_Entry) - 1;
    gdtDesc.entries = entries;
    GDT_Load(&gdtDesc, newCs, newDs);
}