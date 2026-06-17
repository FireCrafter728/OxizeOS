#pragma once

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |-----------------------------------------------------------------------------| //
// | Minimal freestanding LIBSTDC Implementation for the OxizeOS kernel          | //
// | STRING: various memory and string modification and transformation functions | //
// |-----------------------------------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdint.hpp>

#ifndef ASMCALL
#define ASMCALL extern "C"
#endif

// -------------------------------------- //
// MEMORY MODIFICATION AND TRANSFORMATION //
// -------------------------------------- //

void* memset(void* ptr, int value, size_t num);
int memcmp(const void* ptr1, const void* ptr2, size_t num);
void* memcpy(void* dst, const void* src, size_t num);