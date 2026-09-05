// SPDX-License-Identifier: GPL-3.0-or-later

#include <arch/x86_64/Utility/io.hpp>
#include <arch/x86_64/MP/lpdata.hpp>

#include <stdio.hpp>

void HaltSystem()
{
	krnl::LPSpecificData* data = krnl::LPData::GetLPDataForCurrentLP();
	if(!data) printf("System Halted\r\n");
	else printf("Halted LP %lu\r\n", data->identity.lpid);
	HaltSystemImpl();
}