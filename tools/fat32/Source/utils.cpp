// SPDX-License-Identifier: GPL-3.0-or-later

#include <utils.hpp>
#include <defs.hpp>
#include <cstdio>
#include <cstring>

void PrintSectorSeparator(size_t sector)
{
	char buffer[76]; // 75 chars / hex line + null character
	
	for(size_t i = 0; i < 75; i++)
		buffer[i] = '-'; // Pad with dashes

	buffer[75] = '\0';

	char text[32];
	sprintf(text, " Sector %lu ", sector);

	size_t textLength = strlen(text);
	size_t start = (75 - textLength) / 2;

	for(size_t i = 0; i < textLength; i++)
		buffer[start + i] = text[i];

	puts(buffer);
}

void DumpFormattedHex(void* buffer, size_t countAligned, size_t displayOffsetAligned)
{
	uint8_t* u8Buffer = reinterpret_cast<uint8_t*>(buffer);

	static const char HexPrintable[256] =
	{
		'.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
		'.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
		' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
		'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
		'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
		'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
		'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', '.',
		'.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
		'.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
		'.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
		'.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
		'.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
		'.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
		'.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
		'.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.'
	};

	printf("00000000: 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  ASCII\n");
	size_t doffLine = displayOffsetAligned / 16;
	for(size_t i = 0; i < countAligned / 16; i++) 
	{
		if((i * 16 + doffLine * 16) % SECTOR_SIZE == 0) PrintSectorSeparator((i * 16 + doffLine * 16) / SECTOR_SIZE);
		printf("%08lX: ", i * 16 + displayOffsetAligned);
		for(size_t j = 0; j < 16; j++) printf("%02X ", u8Buffer[i * 16 + j]);
		printf(" ");
		for(size_t j = 0; j < 16; j++) printf("%c", HexPrintable[u8Buffer[i * 16 + j]]);
		puts("");
	}
	puts("");
}

uint32_t ComputeCRC32(const void* data, size_t length)
{
	static uint32_t table[256];
	static bool initialized = false;

	if(!initialized)
	{
		for(uint32_t i = 0; i < 256; i++)
		{
			uint32_t crc = i;
			for(int j = 0; j < 8; j++)
				crc = (crc >> 1) ^ (0xEDB88320u & (-(int)(crc & 1)));
			table[i] = crc;
		}
		initialized = true;
	}

	uint32_t crc = 0xFFFFFFFFu;
	const uint8_t* buf = static_cast<const uint8_t*>(data);

	for(size_t i = 0; i < length; i++)
	{
		crc = table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
	}

	return crc ^ 0xFFFFFFFFu;
}

codepoint_t utf8ToCodepoint(const char* str, size_t* bytesRead)
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

codepoint_t utf16ToCodepoint(const char16_t* wstr, size_t* wordsRead)
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

codepoint_t utf32ToCodepoint(const char32_t wc)
{
	if(wc >= 0x10FFFF) return 0xFFFD;

	if(wc >= 0xD800 && wc <= 0xDFFF) return 0xFFFD;

	return wc;
}

size_t codepointToUtf8(codepoint_t cp, char* out)
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

size_t codepointToUtf16(codepoint_t cp, char16_t* out)
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

size_t codepointToUtf32(codepoint_t cp, char32_t* out)
{
	if(cp > 0x10FFFF) return 0;

	if(cp >= 0xD800 && cp <= 0xDFFF) return 0;

	out[0] = (char32_t)cp;
	return 1;
}

std::wstring utf8StringToWideString(const std::string_view str)
{
	std::wstring result;

	for(size_t i = 0; i < str.length();)
	{
		size_t bytesRead = 0;
		codepoint_t cp = utf8ToCodepoint(str.data() + i, &bytesRead);
		if(bytesRead == 0) break;

		if(sizeof(wchar_t) == 2)
		{
			char16_t buffer[2];

			size_t words = codepointToUtf16(cp, buffer);
			if(words == 0) return L"";

			for(size_t j = 0; j < words; j++) result += static_cast<wchar_t>(buffer[j]);
		}
		else if(sizeof(wchar_t) == 4)
		{
			char32_t dword;

			size_t dwords = codepointToUtf32(cp, &dword);
			if(dwords == 0) return L"";

			result += static_cast<wchar_t>(dword);
		}
		else return L"";

		i += bytesRead;
	}

	return result;
}

std::string wideStringToUtf8String(const std::wstring_view wstr)
{
	std::string result;

	for(size_t i = 0; i < wstr.length();)
	{
		size_t wordsRead = 0;
		codepoint_t cp;
		
		if(sizeof(wchar_t) == 2) cp = utf16ToCodepoint(reinterpret_cast<const char16_t*>(wstr.data()) + i, &wordsRead);
		else if(sizeof(wchar_t) == 4) 
		{
			cp = utf32ToCodepoint(wstr[i]);
			wordsRead = 1;
		}
		else return "";

		if(wordsRead == 0) break;

		char buffer[4];
		size_t bytes = codepointToUtf8(cp, buffer);

		if(bytes == 0) return "";

		for(size_t j = 0; j < bytes; j++) result += buffer[j];

		i += wordsRead;
	}

	return result;
}

std::string FormatNumber(uint64_t value)
{
	std::string result = std::to_string(value);

	for(int64_t pos = static_cast<int64_t>(result.length()) - 3; pos > 0; pos -= 3)
		result.insert(static_cast<size_t>(pos), ",");

	return result;
}