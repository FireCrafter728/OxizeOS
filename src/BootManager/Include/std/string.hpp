// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.hpp>
#include <stddef.hpp>

extern "C" void* memset(void* ptr, int val, size_t num);
extern "C" int memcmp(const void* ptr1, const void* ptr2, size_t num);
extern "C" void* memcpy(void* dst, const void* src, size_t num);