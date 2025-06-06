#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Exact-width integer types */
typedef signed char                 int8_t;
typedef unsigned char               uint8_t;

typedef short                       int16_t;
typedef unsigned short              uint16_t;

typedef long int                    int32_t;
typedef unsigned long int           uint32_t;

typedef long long int               int64_t;
typedef unsigned long long int      uint64_t;

/* Minimum-width integer types */
typedef int8_t   int_least8_t;
typedef uint8_t  uint_least8_t;

typedef int16_t  int_least16_t;
typedef uint16_t uint_least16_t;

typedef int32_t  int_least32_t;
typedef uint32_t uint_least32_t;

typedef int64_t  int_least64_t;
typedef uint64_t uint_least64_t;

/* Fastest minimum-width integer types */
typedef int8_t   int_fast8_t;
typedef uint8_t  uint_fast8_t;

typedef int16_t  int_fast16_t;
typedef uint16_t uint_fast16_t;

typedef int32_t  int_fast32_t;
typedef uint32_t uint_fast32_t;

typedef int64_t  int_fast64_t;
typedef uint64_t uint_fast64_t;

/* Integer types capable of holding object pointers */
#if defined(__x86_64__) || defined(_M_X64)
typedef int64_t  intptr_t;
typedef uint64_t uintptr_t;
#else
typedef int32_t  intptr_t;
typedef uint32_t uintptr_t;
#endif

/* Greatest-width integer types */
typedef int64_t  intmax_t;
typedef uint64_t uintmax_t;

/* Limits of exact-width integer types */
#define INT8_MIN    (-128)
#define INT8_MAX    127
#define UINT8_MAX   255u

#define INT16_MIN   (-32768)
#define INT16_MAX   32767
#define UINT16_MAX  65535u

#define INT32_MIN   (-2147483647 - 1)
#define INT32_MAX   2147483647
#define UINT32_MAX  4294967295u

#define INT64_MIN   (-9223372036854775807LL - 1)
#define INT64_MAX   9223372036854775807LL
#define UINT64_MAX  18446744073709551615ULL

#ifdef __cplusplus
} /* extern "C" */
#endif