#pragma once
#include <stdint.h>
#include <stdbool.h>

#define ASMCALL __attribute__((cdecl))

void ASMCALL outb(uint16_t port, uint8_t value);
void ASMCALL outw(uint16_t port, uint16_t value);
uint8_t ASMCALL inb(uint16_t port);
uint16_t ASMCALL inw(uint16_t port);
void HaltSystem();
void Crash();
void iowait();
void ASMCALL CLI();
void ASMCALL STI();