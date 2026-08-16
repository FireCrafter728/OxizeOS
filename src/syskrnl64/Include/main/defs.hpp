// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#define PAGE_ALIGN_UP(addr) (((addr) + 0xFFFULL) & ~(0xFFFULL))
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(0xFFFULL))

#define BLOCK_COUNT(bytes) (((bytes) + 0xFFFULL) / 0x1000ULL)

#define KIBIBYTE 1024ULL
#define MEBIBYTE KIBIBYTE * KIBIBYTE
#define GIBIBYTE MEBIBYTE * KIBIBYTE

#define SECTOR_SIZE 512ULL

#define MapAddr 0xFFFF800000000000ULL

#define PAGE_SIZE 0x1000ULL
#define BLOCK_SIZE PAGE_SIZE

#define ASMCALL extern "C"
#define PACK __attribute__((packed))
#define FPACK __attribute__((packed, aligned(1)))

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
#define IST1_STACK_SIZE 0x4000
#define IST2_STACK_SIZE 0x4000
#define IST3_STACK_SIZE 0x4000
#define LP_INTHANDLER_STACK_SIZE 0x4000

#define TOTAL_SUPPORTED_LPs 512
#define TOTAL_GDT_ENTRIES 1029

#define RING0_CODE_SEGMENT_OFFSET 0x08
#define RING0_DATA_SEGMENT_OFFSET 0x10
#define RING3_CODE_SEGMENT_OFFSET 0x18
#define RING3_DATA_SEGMENT_OFFSET 0x20
#define BSP_TASK_SWITCH_SEGMENT_OFFSET 0x28

// Align any value to some power of 2 alignment value
#define ALIGN_UP(value, align) (((value) + (align) - 1) & ~((align) - 1))