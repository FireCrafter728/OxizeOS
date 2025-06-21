#pragma once

#include "stdarg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int fd_t;

void fputc(char c, fd_t file);
void fputs(const char* str, fd_t file);
void vfprintf(fd_t file, const char* fmt, va_list args);
void fprintf(fd_t file, const char* fmt, ...);

void putc(char c);
void puts(const char* str);
void vprintf(const char* fmt, va_list args);
void printf(const char* fmt, ...);

#ifdef __cplusplus
}
#endif