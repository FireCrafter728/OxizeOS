#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |----------------------------------------------------------------------------------| //
// | Minimal freestanding LIBSTDC Implementation for the OxizeOS kernel               | //
// | STDINT: various integer type typedefs with various lengths and usage conventions | //
// |----------------------------------------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#ifdef __cplusplus

namespace std
{
    typedef unsigned char uint8_t;
    typedef signed char int8_t;
    typedef unsigned short uint16_t;
    typedef signed short int16_t;
    typedef unsigned int uint32_t;
    typedef signed int int32_t;
    typedef unsigned long long int uint64_t;
    typedef signed long long int int64_t;

    typedef uint64_t size_t;
    typedef uint64_t uintptr_t;
    typedef uint64_t flags_t;
}

using uint8_t = std::uint8_t;
using int8_t = std::int8_t ;
using uint16_t = std::uint16_t;
using int16_t = std::int16_t;
using uint32_t = std::uint32_t;
using int32_t = std::int32_t;
using uint64_t = std::uint64_t;
using int64_t = std::int64_t;
using size_t = std::size_t;
using uintptr_t = std::uintptr_t;
using flags_t = std::flags_t;

#else

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned long long int uint64_t;
typedef signed long long int int64_t;

typedef uint64_t size_t;
typedef uint64_t uintptr_t;
typedef uint64_t flags_t;

#endif