// SPDX-License-Identifier: GPL-3.0-or-later

#include <arch/x86_64/std/stdio.hpp>
#include <arch/x86_64/Utility/io.hpp>

void KernelPutc(char c)
{
	outb(0xE9, c);
}