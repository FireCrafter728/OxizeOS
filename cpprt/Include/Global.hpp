#pragma once

typedef unsigned long long int size_t;

extern void cpprt_print(const char* msg);
extern void cpprt_terminate();
extern void* cpprt_malloc(size_t size);
extern void cpprt_free(void* addr);