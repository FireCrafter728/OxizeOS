#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

const char* strchr(const char* str, char chr);
char* strcpy(char* dst, const char* src);
unsigned strlen(const char* str);
int strcmp(const char* a, const char* b);

void* memcpy(void* dst, const void* src, uint32_t num);
void* memset(void* ptr, int value, uint32_t num);
int memcmp(const void* ptr1, const void* ptr2, uint32_t num);
void* SegOffToLinear(void* addr);

#ifdef __cplusplus
}
#endif