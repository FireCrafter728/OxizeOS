// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdlib.hpp>
#include <stdint.hpp>
#include <string.hpp>

// --------------------------- //
// MEMORY MANAGEMENT FUNCTIONS //
// --------------------------- //

extern void* KernelAlloc(size_t size);
extern void KernelFree(void* ptr);

void* kmalloc(size_t size)
{
    return KernelAlloc(size);
}

void* kcalloc(size_t count, size_t size)
{
    size_t total = count * size;

    void* ptr = kmalloc(total);
    if(!ptr) return nullptr;

    memset(ptr, 0, total);
    return ptr;
}

void kfree(void* ptr)
{
    KernelFree(ptr);
}