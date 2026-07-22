// SPDX-License-Identifier: GPL-3.0-or-later

#include <Drivers/Interrupts/idt.hpp>

using namespace SysKrnl64::IDT;

ASMCALL void IDT_Load(IDT_Desc* desc);

IDT_Entry IDT::entries[256] = {};

void IDT::Initialize()
{
    idtDesc =  {sizeof(entries) - 1, entries};
    IDT_Load(&idtDesc);
}

void IDT::SetGate(int interrupt, void* base, uint16_t segmentDescriptor, uint8_t flags, uint8_t interruptIST)
{
    entries[interrupt].BaseLow = (((uintptr_t)base) & 0xFFFF);
    entries[interrupt].BaseMiddle = (((uintptr_t)base >> 16) & 0xFFFF);
    entries[interrupt].BaseHigh = (((uintptr_t)base >> 32) & 0xFFFFFFFF);
    entries[interrupt].SegmentSelector = segmentDescriptor;
    entries[interrupt].IST = interruptIST;
    entries[interrupt].Flags = flags;
    entries[interrupt].Reserved = 0;
}

void IDT::EnableGate(int interrupt)
{
    entries[interrupt].Flags |= IDT_FLAG_PRESENT;
}

void IDT::DisableGate(int interrupt)
{
    entries[interrupt].Flags &= ~IDT_FLAG_PRESENT;
}