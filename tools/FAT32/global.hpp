#pragma once
#include <string.h>
#include <cwchar>

// DEFS

#define SECTOR_SIZE                                 512

#define MAX_ARG_SIZE                                256

#define MBR_MAX_PARTITIONS                          4
#define MBR_PARTITION_BOOTABLE                      0x80
#define MBR_BOOT_SIGNATURE                          0xAA55

// STDINT
// Mixed with Win32API

typedef signed char m_int8_t;
typedef unsigned char m_uint8_t;
typedef signed short m_int16_t;
typedef unsigned short m_uint16_t;
typedef signed long int m_int32_t;
typedef unsigned long int m_uint32_t;
typedef signed long long int m_int64_t;
typedef unsigned long long int m_uint64_t;

typedef signed int INT;
typedef unsigned int UINT;

typedef wchar_t wchar;
typedef m_uint64_t m_uintptr_t;
typedef m_int64_t m_intptr_t;

typedef char* LPSTR;
typedef const char* LPCSTR;
typedef wchar* LPWSTR;
typedef const wchar* LPCWSTR;

inline bool Empty(LPCSTR str)
{
    return !str || *str == '\0';
}

inline bool Empty(LPCWSTR wstr)
{
    return !wstr || *wstr == L'\0';
}

inline void ZeroOut(void* buf, size_t size)
{
    memset(buf, 0, size);
}

LPSTR ClearWhitespace(LPSTR str);

int ConvertToUTF16(LPCSTR src, LPWSTR dest, int maxLen);
size_t NumberToFormattedStr(m_uint32_t value, LPSTR outStr);
void TrimTrailingSlash(LPSTR path);