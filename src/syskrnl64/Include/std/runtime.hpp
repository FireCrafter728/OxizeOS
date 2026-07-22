// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |----------------------------------------------------------------------| //
// | Minimal freestanding LIBSTDC++ Implementation for the OxizeOS kernel | //
// | RUNTIME: Various runtime operators and pre-launch functions          | //
// |----------------------------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdint.hpp>

// ------------ //
// OPERATOR NEW //
// ------------ //

inline void* operator new(unsigned long int, void* ptr) noexcept
{
    return ptr;
}

inline void* operator new[](unsigned long int, void* ptr) noexcept
{
    return ptr;
}

// --------------- //
// OPERATOR DELETE //
// --------------- //

inline void operator delete(void*, void*) {};
inline void operator delete[](void*, void*) {};