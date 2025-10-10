#pragma once
#include <Uefi.h>
#include <cstdarg>
#include <Library/BaseMemoryLib.h>

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed long int int32_t;
typedef unsigned long int uint32_t;
typedef signed long long int int64_t;
typedef unsigned long long int uint64_t;

typedef uint16_t wchar_t;
typedef uint64_t uintptr_t;
typedef uint64_t size_t;

extern EFI_SYSTEM_TABLE* gSystem;

void* MemoryAlloc(size_t alloc);
void MemoryFree(void* addr);

extern uintptr_t CONV_MEMORY_ADDR;
extern const uintptr_t CONV_MEMORY_SIZE;

const wchar_t* AsciiToUnicode(const char* str);

void HaltSystem();

extern "C" void HaltSystemImpl();

extern "C" uint8_t inb(uint16_t port);
extern "C" void outb(uint16_t port, uint8_t value);

extern "C" uint16_t inw(uint16_t port);
extern "C" void outw(uint16_t port, uint16_t value);

extern "C" uint32_t ind(uint16_t port);
extern "C" void outd(uint16_t port, uint32_t value);

extern "C" void* memcpy(void* dst, const void* src, uint32_t num);