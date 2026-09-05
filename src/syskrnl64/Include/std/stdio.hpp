// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Standard system I/O functions

#include <main/defs.hpp>
#include <stdarg.hpp>

// ----------- //
// DEFINITIONS //
// ----------- //

#if defined(__GNUC__) || defined(__clang__)
#define PRINTF_ATTR(fmt_idx, arg_idx) __attribute__((format(printf, fmt_idx, arg_idx)))
#else
#define PRINTF_ATTR(fmt_idx, arg_idx)
#endif

// ------------------ //
// PRINTING FUNCTIONS //
// ------------------ //

void putc(char c);
void puts(const char* str);
void PRINTF_ATTR(1, 2) printf(const char* fmt, ...);
void vprintf(const char* fmt, va_list args);