// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Extra integer types for the x86_64 architecture

#include <arch/x86_64/std/stdint.hpp>

#ifndef offsetof
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif

namespace std
{
	typedef uint64_t size_t;
	typedef uint64_t uintptr_t;
}

using size_t = std::size_t;
using uintptr_t = std::uintptr_t;