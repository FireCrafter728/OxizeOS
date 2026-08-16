// SPDX-License-Identifier: GPL-3.0-or-later

#include <arch/x86_64/Utility/io.hpp>
#include <stdio.hpp>

void HaltSystem()
{
	puts("System Halted\r\n");
	HaltSystemImpl();
}