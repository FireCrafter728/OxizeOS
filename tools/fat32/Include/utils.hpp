// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <defs.hpp>
#include <cstdint>
#include <cstddef>
#include <string>

// Prints a buffer in a formatted hex view, count and displayOffsetAligned must be 16-byte aligned, displayOffsetAligned is the base of the offset displayed in the left of the dump
void DumpFormattedHex(void* buffer, size_t countAligned, size_t displayOffsetAligned);

uint32_t ComputeCRC32(const void* data, size_t length);

codepoint_t utf8ToCodepoint(const char* str, size_t* bytesRead);
codepoint_t utf16ToCodepoint(const char16_t* wstr, size_t* wordsRead);
codepoint_t utf32ToCodepoint(const char32_t wc);
size_t codepointToUtf8(codepoint_t cp, char* out);
size_t codepointToUtf16(codepoint_t cp, char16_t* out);
size_t codepointToUtf32(codepoint_t cp, char32_t* out);

std::wstring utf8StringToWideString(const std::string_view str);
std::string wideStringToUtf8String(const std::wstring_view wstr);

std::string FormatNumber(uint64_t value);