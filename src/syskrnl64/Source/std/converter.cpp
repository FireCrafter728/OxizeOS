// SPDX-License-Identifier: GPL-3.0-or-later

#include <converter.hpp>
#include <string.hpp>

char* stdEx::convertFreqToStr(uint64_t freq, char* bufferOut, size_t* bufferSize)
{
	auto append_u64 = [&](char* out, size_t i, uint64_t v) -> size_t
	{
		char tmp[32];
		size_t n = 0;
		
		do
		{
			tmp[n++] = char('0' + (v % 10));
			v /= 10;
		} while (v);
	
		while (n--)
			out[i++] = tmp[n];
	
		return i;
	};

	auto u64_len = [&](uint64_t v) -> size_t
	{
		size_t n = 1;
		while (v >= 10)
		{
			v /= 10;
			n++;
		}
		return n;
	};

	const char* unit = " Hz";

	uint64_t value = freq;
	uint64_t rem = 0;

	if (freq >= 1000000000ULL)
	{
		value = freq / 1000000000ULL;
		rem   = freq % 1000000000ULL;
		unit  = " GHz";
	}
	else if (freq >= 1000000ULL)
	{
		value = freq / 1000000ULL;
		rem   = freq % 1000000ULL;
		unit  = " MHz";
	}
	else if (freq >= 1000ULL)
	{
		value = freq / 1000ULL;
		rem   = freq % 1000ULL;
		unit  = " kHz";
	}

	uint64_t frac = 0;

	if (rem)
	{
		if (unit[1] == 'G')
			frac = (rem * 100000ULL) / 1000000000ULL;
		else if (unit[1] == 'M')
			frac = (rem * 100000ULL) / 1000000ULL;
		else if (unit[1] == 'k')
			frac = (rem * 100000ULL) / 1000ULL;
	}

	size_t int_len = u64_len(value);
	size_t frac_len = frac ? 6 : 0;
	size_t unit_len = strlen(unit);

	size_t needed = int_len + frac_len + unit_len + 1;

	if (!bufferSize)
		return nullptr;

	if (!bufferOut || *bufferSize < needed)
	{
		*bufferSize = needed;
		return nullptr;
	}

	size_t i = 0;

	i = append_u64(bufferOut, i, value);

	if (frac)
	{
		bufferOut[i++] = '.';

		uint64_t f = frac;
		for (int p = 4; p >= 0; --p)
		{
			uint64_t div = 1;
			for (int k = 0; k < p; k++) div *= 10;

			bufferOut[i++] = char('0' + (f / div) % 10);
		}
	}

	for(size_t j = 0; unit[j]; j++) bufferOut[i++] = unit[j];

	bufferOut[i] = '\0';

	*bufferSize = needed;

	return bufferOut;
}

codepoint_t stdEx::utf8ToCodepoint(const char* str, size_t* bytesRead)
{
	unsigned char c = (unsigned char)str[0];

	if(c < 0x80)
	{
		*bytesRead = 1;
		return c;
	}

	if((c & 0xE0) == 0xC0)
	{
		*bytesRead = 2;
		return ((c & 0x1F) << 6) | ((unsigned char)str[1] & 0x3F);
	}

	if((c & 0xF0) == 0xE0)
	{
		*bytesRead = 3;
		return ((c & 0x0F) << 12) | (((unsigned char)str[1] & 0x3F) << 6) | ((unsigned char)str[2] & 0x3F);
	}

	if((c & 0xF8) == 0xF0)
	{
		*bytesRead = 4;
		return ((c & 0x07) << 18) | (((unsigned char)str[1] & 0x3F) << 12) | (((unsigned char)str[2] & 0x3F) << 6) | ((unsigned char)str[3] & 0x3F);
	}

	*bytesRead = 1;
	return 0xFFFD;
}

codepoint_t stdEx::utf16ToCodepoint(const char16_t* wstr, size_t* wordsRead)
{
	char16_t wc = wstr[0];

	if(wc >= 0xD800 && wc <= 0xDBFF)
	{
		char16_t low = wstr[1];

		if(low >= 0xDC00 && low <= 0xDFFF)
		{
			*wordsRead = 2;
			return 0x10000 + (((wc - 0xD800) << 10) | (low - 0xDC00));
		}

		*wordsRead = 1;
		return 0xFFFD;
	}

	if(wc >= 0xDC00 && wc <= 0xDFFF)
	{
		*wordsRead = 1;
		return 0xFFFD;
	}

	*wordsRead = 1;
	return wc;
}

size_t stdEx::codepointToUtf8(codepoint_t cp, char* out)
{
	if(cp <= 0x7F)
	{
		out[0] = (char)cp;
		return 1;
	}

	if(cp < 0x7FF)
	{
		out[0] = (char)(0xC0 | (cp >> 6));
		out[1] = (char)(0x80 | (cp & 0x3F));
		return 2;
	}

	if(cp <= 0xFFFF)
	{
		if(cp >= 0xD800 && cp <= 0xDFFF) return 0;

		out[0] = (char)(0xE0 | (cp >> 12));
		out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		out[2] = (char)(0x80 | (cp & 0x3F));
		return 3;
	}

	if(cp <= 0x10FFFF)
	{
		out[0] = (char)(0xF0 | (cp >> 18));
		out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
		out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
		out[3] = (char)(0x80 | (cp & 0x3F));
		return 4;
	}
	
	return 0;
}

size_t stdEx::codepointToUtf16(codepoint_t cp, char16_t* out)
{
	if(cp <= 0xFFFF)
	{
		if(cp >= 0xD800 && cp <= 0xDFFF) return 0;
		out[0] = (char16_t)cp;
		return 1;
	}

	if(cp <= 0x10FFFF)
	{
		cp -= 0x10FFFF;

		out[0] = (char16_t)(0xD800 | (cp >> 10));
		out[1] = (char16_t)(0xDC00 | (cp & 0x3FF));

		return 2;
	}

	return 0;
}

std::string stdEx::wideStringToUtf8String(const wchar_t* wstr)
{
	std::string result;

	for(size_t i = 0; i < wcslen(wstr);)
	{
		size_t wordsRead = 0;
		codepoint_t cp = utf16ToCodepoint(reinterpret_cast<const char16_t*>(wstr) + i, &wordsRead);

		if(wordsRead == 0) break;

		char buffer[4];
		size_t bytes = codepointToUtf8(cp, buffer);

		if(bytes == 0) return "";

		for(size_t j = 0; j < bytes; j++) result += buffer[j];

		i += wordsRead;
	}

	return result;
}