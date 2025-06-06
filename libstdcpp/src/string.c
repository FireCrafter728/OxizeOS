#include <string.h>

const char* strchr(const char* str, char chr)
{
    if (str == NULL)
        return NULL;

    while (*str)
    {
        if (*str == chr)
            return str;

        ++str;
    }

    return NULL;
}

char* strcpy(char* dst, const char* src)
{
    char* origDst = dst;

    if (dst == NULL)
        return NULL;

    if (src == NULL)
    {
        *dst = '\0';
        return dst;
    }

    while (*src)
    {
        *dst = *src;
        ++src;
        ++dst;
    }
    
    *dst = '\0';
    return origDst;
}

unsigned strlen(const char* str)
{
    unsigned len = 0;
    while (*str)
    {
        ++len;
        ++str;
    }

    return len;
}

int strcmp(const char* a, const char* b)
{
    if(a == NULL && b == NULL) return 0;
    if(a == NULL || b == NULL) return -1;
    while(*a && *b && *a == *b) {
        a++;
        b++;
    }
    return (*a) - (*b);
}

void* memcpy(void* dst, const void* src, uint32_t num)
{
    uint8_t* u8Dst = (uint8_t *)dst;
    const uint8_t* u8Src = (const uint8_t *)src;

    for (uint32_t i = 0; i < num; i++)
        u8Dst[i] = u8Src[i];

    return dst;
}

void * memset(void * ptr, int value, uint32_t num)
{
    uint8_t* u8Ptr = (uint8_t *)ptr;

    for (uint32_t i = 0; i < num; i++)
        u8Ptr[i] = (uint8_t)value;

    return ptr;
}

int memcmp(const void* ptr1, const void* ptr2, uint32_t num)
{
    const uint8_t* u8Ptr1 = (const uint8_t *)ptr1;
    const uint8_t* u8Ptr2 = (const uint8_t *)ptr2;

    for (uint32_t i = 0; i < num; i++)
        if (u8Ptr1[i] != u8Ptr2[i])
            return 1;

    return 0;
}

void* SegOffToLinear(void* addr)
{
    uint32_t off = (uint32_t)(addr) & 0xFFFF;
    uint32_t seg = (uint32_t)(addr) >> 16;
    return (void*)(seg * 16 + off);
}