// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |-----------------------------------------------------------------------| //
// | Minimal freestanding LIBSTDC Implementation for the OxizeOS kernel    | //
// | STDIO: various I/O functions to interact with the system and hardware | //
// |-----------------------------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdarg.hpp>

// ------------------ //
// PRINTING FUNCTIONS //
// ------------------ //

void putc(char c);
void puts(const char* str);
void printf(const char* fmt, ...);
void vprintf(const char* fmt, va_list args);

// --------------------------- //
// MEMORY MANAGEMENT FUNCTIONS //
// --------------------------- //

void* kmalloc(size_t size);
void* kcalloc(size_t count, size_t size);
void kfree(void* ptr);