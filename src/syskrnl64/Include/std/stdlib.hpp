// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |--------------------------------------------------------------------| //
// | Minimal freestanding LIBSTDC Implementation for the OxizeOS kernel | //
// | STDLIB: various utility functions for interacting with the system  | //
// |--------------------------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

// --------------------------- //
// MEMORY MANAGEMENT FUNCTIONS //
// --------------------------- //

void* kmalloc(size_t size);
void* kcalloc(size_t count, size_t size);
void kfree(void* ptr);