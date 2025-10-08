#pragma once
#include <Uefi.h>

class print
{
public:
	void SetStdout(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* StdOut);
	void PrintChr(char c);
	void PrintWChr(wchar_t wc);
	void Print(const char* str);
	void PrintW(const wchar_t* wstr);
	void ClrScr();
	void printf(const char* fmt, ...);
	void vprintf(const char* fmt, va_list args);
private:
	void vprintf_unsigned(unsigned long long num, int radix, bool uppercase);
	void vprintf_signed(long long number, int radix, bool uppercase);
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* StdOut;
};

extern print Print;

#define putc(chr) Print.PrintChr(chr)
#define putwc(wchr) Print.PrintWChr(wchr)
#define puts(msg) Print.Print(msg)
#define putsw(wmsg) Print.PrintW(wmsg);

void printf(const char* fmt, ...);