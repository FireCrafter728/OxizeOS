#pragma once

#include <SysTable.hpp>

#include <io.hpp>
#include <string.hpp>
#include <stdio.hpp>
#include <algorithm>
#include <vector>
#include <runtime.hpp>

#include <Drivers/Paging/paging.hpp>

#include <Drivers/Interrupts/gdt.hpp>
#include <Drivers/Interrupts/idt.hpp>
#include <Drivers/Interrupts/isr.hpp>
#include <Drivers/Interrupts/isr_mappings.hpp>
#include <Drivers/Interrupts/irq.hpp>
#include <Drivers/Interrupts/handlers.hpp>
#include <Drivers/APIC/apic.hpp>

#include <Drivers/MMD/mmd.hpp>
#include <Drivers/MMD/krnl.hpp>
#include <Drivers/MMD/mmio.hpp>

#include <Drivers/ACPI/acpi.hpp>

#include <Drivers/PCIe/PCIe.hpp>

#include <Drivers/AHCI/ahci.hpp>

#include <Drivers/CPUID/cpuid.hpp>
#include <Drivers/CPUID/msr.hpp>

#define PAGE_ALIGN_UP(addr) (((addr) + 0xFFFULL) & ~(0xFFFULL))
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(0xFFFULL))

#define BLOCK_COUNT(bytes) (((bytes) + 0xFFFULL) / 0x1000ULL)

#define KIBIBYTE 1024
#define MEBIBYTE KIBIBYTE * KIBIBYTE
#define GIBIBYTE MEBIBYTE * KIBIBYTE

#define SECTOR_SIZE 512

namespace SysKrnl64
{
    constexpr uintptr_t MapAddr = 0xFFFF800000000000;
    constexpr size_t PAGE_SIZE = 0x1000;

    extern MMD::MMD* mmd;
    extern Paging::Paging* paging;
    extern MSR::MSR* msr;
}