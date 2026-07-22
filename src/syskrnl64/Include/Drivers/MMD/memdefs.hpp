// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.hpp>

namespace SysKrnl64
{
    namespace MMD
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
}

#ifndef PACK
#define PACK __attribute__((PACKED))
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif

#ifndef BLOCK_SIZE
#define BLOCK_SIZE PAGE_SIZE
#endif