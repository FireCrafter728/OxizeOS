// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>
#include <arch/x86_64/std/stdint.hpp>

namespace krnl
{
	enum MemoryAllocErrors : uint32_t
	{
		MMD_SUCCESS = 0,
		MMD_OUT_OF_MEMORY = 1,
		MMD_INVALID_PARAMETER = 2,
		MMD_INVALID_INIT_DATA = 3,
		MMD_INVALID_STRUCTURE_DATA = 4,
	};
}