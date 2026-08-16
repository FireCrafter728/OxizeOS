// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Standard system I/O functions

#include <main/defs.hpp>
#include <stdarg.hpp>

// ------------------ //
// PRINTING FUNCTIONS //
// ------------------ //

void putc(char c);
void puts(const char* str);
void printf(const char* fmt, ...);
void vprintf(const char* fmt, va_list args);