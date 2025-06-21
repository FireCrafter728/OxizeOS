#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define ASMCALL __attribute__((cdecl))

void ASMCALL outb(uint16_t port, uint8_t value);
uint8_t ASMCALL inb(uint16_t port);
void ASMCALL outw(uint16_t port, uint16_t value);
uint16_t ASMCALL inw(uint16_t port);
void ASMCALL outd(uint16_t port, uint32_t value);
uint32_t ASMCALL ind(uint16_t port);
void HaltSystem();
void ASMCALL CLI();
void ASMCALL STI();
void ASMCALL Interrupt();
void iowait();
uint64_t ASMCALL rdmsr(uint32_t msr);
void ASMCALL wrmsr(uint32_t msr, uint64_t value);
void ASMCALL sleep(uint32_t iterations);

#ifdef __cplusplus
}
#endif