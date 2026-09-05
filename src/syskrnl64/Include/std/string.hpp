// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifndef ASMCALL
#define ASMCALL extern "C"
#endif

// -------------------------------------- //
// MEMORY MODIFICATION AND TRANSFORMATION //
// -------------------------------------- //

void* memset(void* ptr, int value, size_t num);
int memcmp(const void* ptr1, const void* ptr2, size_t num);
void* memcpy(void* dst, const void* src, size_t num);
void* memmove(void* dst, const void* src, size_t num);

// --------------------------- //
// CHARACTER STRING OPERATIONS //
// --------------------------- //

size_t strlen(const char* str);
int strcmp(const char* str1, const char* str2);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, size_t n);

// -------------------------------- //
// WIDE CHARACTER STRING OPERATIONS //
// -------------------------------- //

size_t wcslen(const wchar_t* wstr);