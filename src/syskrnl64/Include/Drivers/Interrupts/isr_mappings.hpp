#pragma once

#include <stdint.hpp>

// ISR Mappings

// ISR Range: 0-255(0x00-0xFF)

// ISRs 0-31(0x00-0x1F): CPU Exceptions, mustn't be overriden

// ISRs 32-127(0x20-0x7F): Reserved for IRQs
constexpr uint8_t IRQ_BASE = 0x20;
constexpr uint8_t IRQ_COUNT = 0x60;

// IRQ 0: Interrupt Timer
constexpr uint8_t IRQ_PIT = 0;
// IRQ 1: PS/2 Keyboard controller
constexpr uint8_t IRQ_KBD = 1;
// IRQ 2-95: Undefined

// ISRs 128-175(0x80-0xAF): Reserved for software
constexpr uint8_t ISR_SYSCALL = 0x80;

// ISRs 176-255(0xB0-0xFE): Reserved for MSI/MSI-X interrupts

// ISR 255(0xFF): Spurious interrupt handler
constexpr uint8_t ISR_SVR = 0xFF;