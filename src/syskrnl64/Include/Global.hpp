#pragma once

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |-------------------------------------------------------------------------------------------------------------| //
// | OxizeOS Kernel Implementation                                                                               | //
// | GLOBAL: Forced include, contains includes, definitions and global variables used accross the implementation | //
// |-------------------------------------------------------------------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <SysTable.hpp>

#include <io.hpp>
#include <string.hpp>
#include <stdio.hpp>
#include <algorithm>
#include <expected>
#include <vector>
#include <runtime.hpp>

#include <Drivers/Paging/paging.hpp>

#include <Drivers/MMD/phys.hpp>
#include <Drivers/MMD/virt.hpp>
#include <Drivers/MMD/heap.hpp>

#include <Drivers/Interrupts/gdt.hpp>
#include <Drivers/Interrupts/idt.hpp>
#include <Drivers/Interrupts/isr.hpp>
#include <Drivers/Interrupts/isr_mappings.hpp>
#include <Drivers/Interrupts/irq.hpp>
#include <Drivers/Interrupts/handlers.hpp>
#include <Drivers/APIC/apic.hpp>

#include <Drivers/ACPI/acpi.hpp>

#include <Drivers/PCIe/PCIe.hpp>

#include <Drivers/AHCI/ahci.hpp>

#include <Drivers/CPUID/cpuid.hpp>
#include <Drivers/CPUID/msr.hpp>

#define PAGE_ALIGN_UP(addr) (((addr) + 0xFFFULL) & ~(0xFFFULL))
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(0xFFFULL))

#define BLOCK_COUNT(bytes) (((bytes) + 0xFFFULL) / 0x1000ULL)

#define KIBIBYTE 1024ULL
#define MEBIBYTE KIBIBYTE * KIBIBYTE
#define GIBIBYTE MEBIBYTE * KIBIBYTE

#ifndef SECTOR_SIZE
#define SECTOR_SIZE 512ULL
#endif

#define MapAddr 0xFFFF800000000000ULL

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000ULL
#endif

#define USERSPACE_VADDR_START 0x0000000000400000ULL // Reserve address space 0-0x3FFFFF for null pointer catching
#define USERSPACE_VADDR_END 0x00007FFFFFFFFFFFULL
#define KERNEL_VADDR_START 0xFFFF800000000000ULL
#define KERNEL_VADDR_END 0xFFFFFFFFFFFFFFFFULL

#define UCHAR_MAX 0xFF
#define USHORT_MAX 0xFFFF
#define ULONG_MAX 0xFFFFFFFF
#define ULONG64_MAX 0xFFFFFFFFFFFFFFFF

#define UINT8_MAX UCHAR_MAX
#define UINT16_MAX USHORT_MAX
#define UINT32_MAX ULONG_MAX
#define UINT64_MAX ULONG64_MAX

#define KERNEL_STACK_SIZE 0x80000 // 512KiB kernel stack

// Align any value to some power of 2 alignment value
#define ALIGN_UP(value, align) (((value) + (align) - 1) & ~((align) - 1))

namespace SysKrnl64
{
    extern Paging::Paging* paging;
    extern MSR::MSR* msr;
    extern MMD::PhysAlloc* physAlloc;
    extern MMD::VirtAlloc* virtAlloc;
    extern MMD::HeapAlloc* heapAlloc;
}