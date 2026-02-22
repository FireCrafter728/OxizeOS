#include <stdio.hpp>

using namespace BootMgr;

static uint8_t ConvBuffer[PAGE_SIZE];

const wchar_t* AsciiToUTF16(const char* str)
{
	if(!str) return nullptr;

	size_t maxChars = sizeof(ConvBuffer) / sizeof(wchar_t) - 1;
	size_t i = 0;
	wchar_t* buffer = reinterpret_cast<wchar_t*>(ConvBuffer);


	while(str[i] && i < maxChars)
	{
		uint8_t c = (uint8_t)str[i];
		buffer[i] = (c <= 0x7F) ? (wchar_t)c : L'?';
		i++;
	}

	buffer[i] = L'\0';
	return buffer;
}

void clrscr()
{
	gSystem->ConOut->ClearScreen(gSystem->ConOut);
}

void putc(char c)
{
	CHAR16 chr[2];
	chr[0] = (CHAR16)c;
	chr[1] = L'\0';

	gSystem->ConOut->OutputString(gSystem->ConOut, chr);
}

void wputc(wchar_t c)
{
	CHAR16 chr[2];
	chr[0] = c;
	chr[1] = L'\0';

	gSystem->ConOut->OutputString(gSystem->ConOut, chr);
}

void wputs(const wchar_t* wstr)
{
	gSystem->ConOut->OutputString(gSystem->ConOut, (CHAR16*)wstr);
}

void puts(const char* str)
{
	const wchar_t* wstr = AsciiToUTF16(str);
	wputs(wstr);
}

const char HexChars[] = "0123456789abcdef";
const char HexCharsUpper[] = "0123456789ABCDEF";

void vprintf_unsigned(unsigned long long number, int radix, bool upper)
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

void vprintf_signed(long long number, int radix , bool upper)
{
    if (number < 0)
    {
        putc('-');
        vprintf_unsigned(-number, radix, upper);
    }
    else vprintf_unsigned(number, radix, upper);
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

void printf(const char* fmt, ...)
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

                    case 'X':	radix = 16; sign = false; number = true; upper = true; break;
                    case 'x':
                    case 'p':   radix = 16; sign = false; number = true;
                                break;

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
                        case PRINTF_LENGTH_DEFAULT:     vprintf_signed(va_arg(args, int), radix, upper);
                                                        break;

                        case PRINTF_LENGTH_LONG:        vprintf_signed(va_arg(args, long), radix, upper);
                                                        break;

                        case PRINTF_LENGTH_LONG_LONG:   vprintf_signed(va_arg(args, long long), radix, upper);
                                                        break;
                        }
                    }
                    else
                    {
                        switch (length)
                        {
                        case PRINTF_LENGTH_SHORT_SHORT:
                        case PRINTF_LENGTH_SHORT:
                        case PRINTF_LENGTH_DEFAULT:     vprintf_unsigned(va_arg(args, unsigned int), radix, upper);
                                                        break;
                                                        
                        case PRINTF_LENGTH_LONG:        vprintf_unsigned(va_arg(args, unsigned  long), radix, upper);
                                                        break;

                        case PRINTF_LENGTH_LONG_LONG:   vprintf_unsigned(va_arg(args, unsigned  long long), radix, upper);
                                                        break;
                        }
                    }
                }

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