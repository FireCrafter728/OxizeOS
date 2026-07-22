// SPDX-License-Identifier: GPL-3.0-or-later

#include <string.hpp>

// Optimized memset function operating in quad-words
void* memset(void* ptr, int value, size_t num)
{
    // Pad the buffer until the address becomes 8 byte aligned
    uint8_t* u8Ptr = reinterpret_cast<uint8_t*>(ptr);
    uint8_t val8 = static_cast<uint8_t>(value);

    uint8_t bytesToPad = (-reinterpret_cast<uintptr_t>(ptr)) & 7;
    if(bytesToPad > num) bytesToPad = num;

    for(uint8_t pb = 0; pb < bytesToPad; pb++) u8Ptr[pb] = val8;
    num -= bytesToPad;

    if(num == 0) return ptr;

    // Fill memory in quad-words for extra performance
    uint64_t* u64Ptr = reinterpret_cast<uint64_t*>(u8Ptr + bytesToPad);

    uint64_t val64 = 0x0101010101010101ULL * static_cast<uint64_t>(val8);
    uint64_t count64 = num >> 3; // Avoid division as it's slower

    for(uint64_t q = 0; q < count64; q++)u64Ptr[q] = val64;

    // Fill the remainder in bytes
    uint8_t rem = num & 7; // Avoid mod instruction as it's slower

    u8Ptr = reinterpret_cast<uint8_t*>(u64Ptr + count64);

    for(uint8_t rb = 0; rb < rem; rb++) u8Ptr[rb] = val8;

    return ptr;
}

int memcmp(const void* ptr1, const void* ptr2, size_t num)
{
    const uint8_t* u8Ptr1 = reinterpret_cast<const uint8_t*>(ptr1);
    const uint8_t* u8Ptr2 = reinterpret_cast<const uint8_t*>(ptr2);

    for(size_t i = 0; i < num; i++)if(u8Ptr1[i] != u8Ptr2[i]) return (int)u8Ptr1[i] - (int)u8Ptr2[i];

    return 0;
}

// Optimized memcpy function operating in quad words
void* memcpy(void* dst, const void* src, size_t num)
{
    // Copy to the dest in bytes until the dest ptr becomes 8 byte aligned
    uint8_t* u8Dst = reinterpret_cast<uint8_t*>(dst);
    const uint8_t* u8Src = reinterpret_cast<const uint8_t*>(src);

    uint8_t bytesToPad = (-reinterpret_cast<uintptr_t>(u8Dst)) & 7;
    if(bytesToPad > num) bytesToPad = num;

    for(uint8_t pb = 0; pb < bytesToPad; pb++) u8Dst[pb] = u8Src[pb];

    u8Dst += bytesToPad;
    u8Src += bytesToPad;
    num -= bytesToPad;

    if(num == 0) return dst;

    // Copy to dest in quad-words for extra performance
    uint64_t* u64Dst = reinterpret_cast<uint64_t*>(u8Dst);
    const uint64_t* u64Src = reinterpret_cast<const uint64_t*>(u8Src); // Do not care if src might be misaligned

    uint64_t count64 = num >> 3; // Avoid division as it's slower

    for(uint64_t q = 0; q < count64; q++)u64Dst[q] = u64Src[q];

    // copy the remainder in bytes
    uint8_t rem = num & 7; // Avoid mod instruction as it's slower

    u8Dst = reinterpret_cast<uint8_t*>(u64Dst + count64);
    u8Src = reinterpret_cast<const uint8_t*>(u64Src + count64);

    for(uint8_t rb = 0; rb < rem; rb++) u8Dst[rb] = u8Src[rb];

    return dst;
}

size_t strlen(const char* str)
{
    size_t len = 0;
    while(*str)
    {
        len++;
        str++;
    }

    return len;
}