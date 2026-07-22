// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.hpp>
#include <io.hpp>
#include <stdint.hpp>

// ----------- //
// DEFINITIONS //
// ----------- //

#if defined(__GNUC__) || defined(__clang__)
#define PRINTF_ATTR(fmt_idx, arg_idx) __attribute__((format(printf, fmt_idx, arg_idx)))
#else
#define PRINTF_ATTR(fmt_idx, arg_idx)
#endif

// ------------------ //
// PRINTING FUNCTIONS //
// ------------------ //

void putc(char c)
{
    outb(0xE9, c);
}

void puts(const char* str)
{
    while(*str)
    {
        putc(*str);
        str++;
    }
}

const char HexChars[] = "0123456789abcdef";
const char HexCharsUpper[] = "0123456789ABCDEF";

void printf_unsigned(uint64_t number, int radix, bool upper)
{
    char buffer[32];
    int pos = 0;

    do 
    {
        unsigned long long rem = number % radix;
        number /= radix;
        buffer[pos++] = upper ? HexCharsUpper[rem] : HexChars[rem];
    } while (number > 0);

    while (--pos >= 0)
        putc(buffer[pos]);
}

void printf_signed(int64_t number, int radix, bool upper)
{
    if (number < 0)
    {
        putc('-');
        printf_unsigned(-number, radix, upper);
    }
    else printf_unsigned(number, radix, upper);
}

#define PRINTF_STATE_NORMAL         0
#define PRINTF_STATE_LENGTH         1
#define PRINTF_STATE_LENGTH_SHORT   2
#define PRINTF_STATE_LENGTH_LONG    3
#define PRINTF_STATE_SPEC           4

#define PRINTF_LENGTH_DEFAULT       0
#define PRINTF_LENGTH_SHORT_SHORT   1
#define PRINTF_LENGTH_SHORT         2
#define PRINTF_LENGTH_LONG          3
#define PRINTF_LENGTH_LONG_LONG     4

void PRINTF_ATTR(1, 2) printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void vprintf(const char* fmt, va_list args)
{
    int state = PRINTF_STATE_NORMAL;
    int length = PRINTF_LENGTH_DEFAULT;
    int radix = 10;
    bool sign = false;
    bool number = false;
    bool upper = false;

    while (*fmt)
    {
        switch (state)
        {
            case PRINTF_STATE_NORMAL:
                switch (*fmt)
                {
                    case '%':   state = PRINTF_STATE_LENGTH;
                                break;
                    default:    putc(*fmt);
                                break;
                }
                break;

            case PRINTF_STATE_LENGTH:
                switch (*fmt)
                {
                    case 'h':   length = PRINTF_LENGTH_SHORT;
                                state = PRINTF_STATE_LENGTH_SHORT;
                                break;
                    case 'l':   length = PRINTF_LENGTH_LONG;
                                state = PRINTF_STATE_LENGTH_LONG;
                                break;
                    default:    goto PRINTF_STATE_SPEC_;
                }
                break;

            case PRINTF_STATE_LENGTH_SHORT:
                if (*fmt == 'h')
                {
                    length = PRINTF_LENGTH_SHORT_SHORT;
                    state = PRINTF_STATE_SPEC;
                }
                else goto PRINTF_STATE_SPEC_;
                break;

            case PRINTF_STATE_LENGTH_LONG:
                if (*fmt == 'l')
                {
                    length = PRINTF_LENGTH_LONG_LONG;
                    state = PRINTF_STATE_SPEC;
                }
                else goto PRINTF_STATE_SPEC_;
                break;

            case PRINTF_STATE_SPEC:
            PRINTF_STATE_SPEC_:
                switch (*fmt)
                {
                    case 'c':   putc((char)va_arg(args, int));
                                break;

                    case 's':   
                                puts(va_arg(args, const char*));
                                break;

                    case '%':   putc('%');
                                break;

                    case 'd':
                    case 'i':   radix = 10; sign = true; number = true;
                                break;

                    case 'u':   radix = 10; sign = false; number = true;
                                break;

                    case 'X':   radix = 16; sign = false; number = true; upper = true; break;
                    case 'x':   radix = 16; sign = false; number = true;
                                break;
                    case 'p': 
                    {
                        uintptr_t ptr = reinterpret_cast<uintptr_t>(va_arg(args, void*));
                        puts("0x");
                        printf_unsigned(ptr, 16, true);
                        break;
                    }

                    case 'o':   radix = 8; sign = false; number = true;
                                break;

                    default:    break;
                }

                if (number)
                {
                    if (sign)
                    {
                        switch (length)
                        {
                        case PRINTF_LENGTH_SHORT_SHORT:
                        case PRINTF_LENGTH_SHORT:
                        case PRINTF_LENGTH_LONG:
                        case PRINTF_LENGTH_DEFAULT:     printf_signed(va_arg(args, int), radix, upper);
                                                        break;

                        case PRINTF_LENGTH_LONG_LONG:   printf_signed(va_arg(args, int64_t), radix, upper);
                                                        break;
                        }
                    }
                    else
                    {
                        switch (length)
                        {
                        case PRINTF_LENGTH_SHORT_SHORT:
                        case PRINTF_LENGTH_SHORT:
                        case PRINTF_LENGTH_DEFAULT:
                        case PRINTF_LENGTH_LONG:        printf_unsigned(va_arg(args, uint32_t), radix, upper);
                                                        break;
                                                        

                        case PRINTF_LENGTH_LONG_LONG:   printf_unsigned(va_arg(args, uint64_t), radix, upper);
                                                        break;
                        }
                    }
                }

                // reset state
                state = PRINTF_STATE_NORMAL;
                length = PRINTF_LENGTH_DEFAULT;
                radix = 10;
                sign = false;
                number = false;
                upper = false;
                break;
        }

        fmt++;
    }

    va_end(args);
}