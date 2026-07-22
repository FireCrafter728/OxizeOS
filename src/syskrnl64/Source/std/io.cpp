// SPDX-License-Identifier: GPL-3.0-or-later

#include <io.hpp>
#include <stdio.hpp>

void HaltSystem()
{
    puts("System Halted\r\n");
    HaltSystemImpl();
}