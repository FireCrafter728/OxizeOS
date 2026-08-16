// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>
#include <stddef.h>

// --------------------------- //
// MEMORY MANAGEMENT FUNCTIONS //
// --------------------------- //

void* kmalloc(size_t size);
void* kcalloc(size_t count, size_t size);
void kfree(void* ptr);