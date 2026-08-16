// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>
#include <string>

typedef uint32_t codepoint_t;

namespace stdEx
{
	char* convertFreqToStr(uint64_t freq, char* bufferOut, size_t* bufferSize);

	codepoint_t utf8ToCodepoint(const char* str, size_t* bytesRead);
	codepoint_t utf16ToCodepoint(const char16_t* wstr, size_t* wordsRead);
	size_t codepointToUtf8(codepoint_t cp, char* out);
	size_t codepointToUtf16(codepoint_t cp, char16_t* out);
	std::string wideStringToUtf8String(const wchar_t* wstr);
}