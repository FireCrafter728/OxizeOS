#pragma once

#include <SysTable.hpp>

#include <io.hpp>
#include <string.hpp>
#include <stdio.hpp>

#include <Drivers/Paging/paging.hpp>

#include <Drivers/Interrupts/gdt.hpp>
#include <Drivers/Interrupts/idt.hpp>
#include <Drivers/Interrupts/isr.hpp>

#define PAGE_ALIGN_UP(addr) (((addr) + 0xFFFULL) & ~(0xFFFULL))
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(0xFFFULL))

namespace TskSchl
{
    constexpr uintptr_t MapAddr = 0xFFFF800000000000;
    constexpr size_t PAGE_SIZE = 0x1000;
}