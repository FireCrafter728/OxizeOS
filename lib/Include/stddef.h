#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t size_t;
typedef int ptrdiff_t;

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef offsetof
#define offsetof(type, member) ((size_t) &(((type *)0)->member))
#endif

#ifdef __cplusplus
}
#endif
