#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void clrscr();
void putc(char c);
void puts(const char* str);
void printf(const char* fmt, ...);
void print_buffer(const char* msg, const void* buffer, uint32_t count);

#ifdef __cplusplus
}
#endif