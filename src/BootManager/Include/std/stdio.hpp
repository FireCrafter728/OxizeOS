#pragma once
#include <stdarg.hpp>

void clrscr();
void putc(char c);
void wputc(wchar_t c);
void puts(const char* str);
void wputs(const wchar_t* wstr);
void printf(const char* fmt, ...);
void vprintf(const char* fmt, va_list args);