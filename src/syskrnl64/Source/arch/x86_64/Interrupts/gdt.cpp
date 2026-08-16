// SPDX-License-Identifier: GPL-3.0-or-later

#include <arch/x86_64/Interrupts/gdt.hpp>

#include <arch/x86_64/std/stddef.hpp>

using namespace krnl;

ASMCALL void GDT_Load(GDT_Desc* desc, uint16_t newCs, uint16_t newDs);

void GDT::Initialize(GDT_Entry* entries, size_t entryCount, uint16_t newCs, uint16_t newDs)
{
	gdtDesc.Limit = entryCount * sizeof(GDT_Entry) - 1;
	gdtDesc.entries = entries;
	GDT_Load(&gdtDesc, newCs, newDs);
}