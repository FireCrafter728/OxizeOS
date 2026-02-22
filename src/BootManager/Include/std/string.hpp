#pragma once
#include <stdint.hpp>

int memcmp(const void* ptr1, const void* ptr2, size_t num);

extern "C" void* memset(void* ptr, uint8_t val, size_t num);
extern "C" void* memcpy(void* dst, const void* src, size_t num);