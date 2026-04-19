#pragma once
#include <stdint.hpp>

#ifndef ASMCALL
#define ASMCALL extern "C"
#endif

ASMCALL void* memset(void* ptr, int value, size_t num);

constexpr int memcmp(const void* ptr1, const void* ptr2, size_t num) {
    return __builtin_memcmp(ptr1, ptr2, num);
}

constexpr void* memcpy(void* dst, const void* src, size_t num) {
    return __builtin_memcpy(dst, src, num);
}