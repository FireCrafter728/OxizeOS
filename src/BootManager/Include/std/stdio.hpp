// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdarg.hpp>

#if defined(__GNUC__) || defined(__clang__)
#define PRINTF_ATTR(fmt_idx, arg_idx) __attribute__((format(printf, fmt_idx, arg_idx)))
#else
#define PRINTF_ATTR(fmt_idx, arg_idx)
#endif

void clrscr();
void putc(char c);
void wputc(wchar_t c);
void puts(const char* str);
void wputs(const wchar_t* wstr);
void PRINTF_ATTR(1, 2) printf(const char* fmt, ...);
void vprintf(const char* fmt, va_list args);