#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

void* memcpy(void* dst, const void* src, uint32_t num);
void* memset(void* ptr, int value, uint32_t num);
int memcmp(const void* ptr1, const void* ptr2, uint32_t num);
void* SegOffToLinear(void* addr);

#ifdef __cplusplus
}
#endif