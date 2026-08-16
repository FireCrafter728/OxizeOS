// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Include the correct stddef implementing header based off the architecture

#if defined(__x86_64__) || defined(__amd64__)

#include <arch/x86_64/std/stddef.hpp>

#endif

