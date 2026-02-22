#pragma once
#include <stdint.hpp>

inline void* memset(void* ptr, int value, size_t num) {
    return __builtin_memset(ptr, value, num);
}