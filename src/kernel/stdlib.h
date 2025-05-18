#pragma once
#include <stddef.h>

void qsort(void* base, size_t num, size_t size, int (*compare)(const void*, const void*));