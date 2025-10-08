#include <print.hpp>

void print::SetStdout(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* StdOut)
{
	this->StdOut = StdOut;
}

void print::PrintChr(char c)
{
	PrintWChr((CHAR16)c);
}

void print::PrintWChr(wchar_t wc)
{
	CHAR16 str[2] = { wc, 0 };
	StdOut->OutputString(StdOut, str);
}

void print::Print(const char* str)
{
	StdOut->OutputString(StdOut, (CHAR16*)AsciiToUnicode(str));
}

void print::PrintW(const wchar_t* wstr)
{
	StdOut->OutputString(StdOut, (CHAR16*)wstr);
}

void print::ClrScr()
{
	StdOut->ClearScreen(StdOut);
}

void print::printf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

#define PRINTF_STATE_NORMAL 0
#define PRINTF_STATE_LENGTH 1
#define PRINTF_STATE_LENGTH_SHORT 2
#define PRINTF_STATE_LENGTH_LONG 3
#define PRINTF_STATE_SPEC 4

#define PRINTF_LENGTH_DEFAULT 0
#define PRINTF_LENGTH_SHORT_SHORT 1
#define PRINTF_LENGTH_SHORT 2
#define PRINTF_LENGTH_LONG 3
#define PRINTF_LENGTH_LONG_LONG 4

const char HexChars[] = "0123456789abcdef";
const char HexCharsUpper[] = "0123456789ABCDEF";

void print::vprintf_unsigned(unsigned long long num, int radix, bool uppercase)
{
	char buf[32];
	int pos = 0;

	do {
		unsigned long long rem = num % radix;
		num /= radix;
		buf[pos++] = uppercase ? HexCharsUpper[rem] : HexChars[rem];
	} while (num > 0);

	while (--pos >= 0) PrintChr(buf[pos]);
}

void print::vprintf_signed(long long number, int radix, bool uppercase)
{
	if (number < 0)
	{
		PrintChr('-');
		vprintf_unsigned(-number, radix, uppercase);
	}
	else vprintf_unsigned(number, radix, uppercase);
}

void print::vprintf(const char* fmt, va_list args)
{
	int state = PRINTF_STATE_NORMAL;
	int length = PRINTF_LENGTH_DEFAULT;
	int radix = 10;
	bool sign = false;
	bool number = false;
	bool uppercase = false;

	while (*fmt)
	{
		switch (state)
		{
		case PRINTF_STATE_NORMAL:
		{
			switch (*fmt)
			{
			case '%': state = PRINTF_STATE_LENGTH; break;
			default: PrintChr(*fmt); break;
			}
			break;
		}
		case PRINTF_STATE_LENGTH:
		{
			switch (*fmt)
			{
			case 'h': length = PRINTF_LENGTH_SHORT; state = PRINTF_STATE_LENGTH_SHORT; break;
			case 'l': length = PRINTF_LENGTH_LONG; state = PRINTF_STATE_LENGTH_LONG; break;
			default: goto PRINTF_STATE_SPEC_;
			}
			break;
		}
		case PRINTF_STATE_LENGTH_SHORT:
		{
			if (*fmt == 'h')
			{
				length = PRINTF_LENGTH_SHORT_SHORT;
				state = PRINTF_STATE_SPEC;
			}
			else goto PRINTF_STATE_SPEC_;
			break;
		}
		case PRINTF_STATE_LENGTH_LONG:
		{
			if (*fmt == 'l')
			{
				length = PRINTF_LENGTH_LONG_LONG;
				state = PRINTF_STATE_SPEC;
			}
			else goto PRINTF_STATE_SPEC_;
			break;
		}
		case PRINTF_STATE_SPEC:
		PRINTF_STATE_SPEC_:
		{
			switch (*fmt)
			{
			case 'c': PrintChr((char)va_arg(args, int)); break;
			case 's': Print(va_arg(args, const char*)); break;
			case '%': PrintChr('%'); break;
			case 'd':
			case 'i': radix = 10; sign = true; number = true; break;
			case 'u': radix = 10; sign = false; number = true; break;
			case 'X': radix = 16; sign = false; uppercase = true; number = true; break;
			case 'x':
			case 'p': radix = 16; sign = false; number = true; break;
			default: break;
			}

			if (number)
			{
				if (sign)
				{
					switch (length)
					{
					case PRINTF_LENGTH_SHORT_SHORT:
					case PRINTF_LENGTH_SHORT:
					case PRINTF_LENGTH_DEFAULT: vprintf_signed(va_arg(args, int), radix, uppercase); break;
					case PRINTF_LENGTH_LONG: vprintf_signed(va_arg(args, long), radix, uppercase); break;
					case PRINTF_LENGTH_LONG_LONG: vprintf_signed(va_arg(args, long long), radix, uppercase); break;
					}
				}
				else {
					switch (length)
					{
					case PRINTF_LENGTH_SHORT_SHORT:
					case PRINTF_LENGTH_SHORT:
					case PRINTF_LENGTH_DEFAULT: vprintf_unsigned(va_arg(args, unsigned int), radix, uppercase); break;
					case PRINTF_LENGTH_LONG: vprintf_unsigned(va_arg(args, unsigned long), radix, uppercase); break;
					case PRINTF_LENGTH_LONG_LONG: vprintf_unsigned(va_arg(args, unsigned long long), radix, uppercase); break;
					}
				}
			}

			state = PRINTF_STATE_NORMAL;
			length = PRINTF_LENGTH_DEFAULT;
			radix = 10;
			sign = false;
			number = false;
			break;
		}
		}
		fmt++;
	}
}

void printf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	Print.vprintf(fmt, args);
	va_end(args);
}
