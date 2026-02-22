#pragma once

#include <stdarg.hpp>

void putc(char c);
void puts(const char* str);
void printf(const char* fmt, ...);
void vprintf(const char* fmt, va_list args);