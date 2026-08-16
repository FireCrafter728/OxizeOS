// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>
#include <stddef.h>

// ------------ //
// OPERATOR NEW //
// ------------ //

inline void* operator new(size_t, void* ptr) noexcept
{
	return ptr;
}

inline void* operator new[](size_t, void* ptr) noexcept
{
	return ptr;
}

// --------------- //
// OPERATOR DELETE //
// --------------- //

inline void operator delete(void*, void*) {};
inline void operator delete[](void*, void*) {};