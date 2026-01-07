#include <global.hpp>
#include <ctype.h>

LPSTR ClearWhitespace(LPSTR str)
{
    if(!str) return nullptr;

    LPSTR res = str;
    while(*str)
    {
        if(!isspace((m_uint8_t)*str)) *res++ = *str;
        str++;
    }
    *res = '\0';
    return res;
}

int ConvertToUTF16(LPCSTR src, LPWSTR dest, int maxLen)
{
    int len = 0;
    for(; *src && len < maxLen; src++, len++) dest[len] = (wchar)(m_uint8_t)(*src);
    dest[len] = 0;
    return len;
}

size_t NumberToFormattedStr(m_uint32_t value, LPSTR outStr)
{
    if(!outStr) return 0;

    char buffer[20];
    size_t pos = 0;
    size_t digitCount = 0;

    if(value == 0)
    {
        outStr[0] = '0';
        outStr[1] = '\0';
        return 1;
    }

    while(value > 0)
    {
        if(digitCount > 0 && digitCount % 3 == 0)
        {
            buffer[pos++] = ',';
        }

        buffer[pos++] = '0' + (value % 10);
        value /= 10;
        digitCount++;
    }

    size_t len = pos;
    for(size_t i = 0; i < len; i++) outStr[i] = buffer[len - 1 - i];
    outStr[len] = '\0';
    return len;
}

void TrimTrailingSlash(LPSTR path) {
    size_t len = strlen(path);
    while(len > 1 && (path[len - 1] == '/' || path[len - 1] == '\\')) {
        path[len - 1] = '\0';
        len--;
    }
}