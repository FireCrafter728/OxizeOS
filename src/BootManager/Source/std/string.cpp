// SPDX-License-Identifier: GPL-3.0-or-later
//
// OxizeOS Operating System for the x86 amd64(x86_64) architecture
// Copyright (C) 2025-2026 FireCrafter728
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <string.hpp>
#include <stdio.hpp>

int memcmp(const void* ptr1, const void* ptr2, size_t num)
{
	const uint8_t* u8Ptr1 = reinterpret_cast<const uint8_t*>(ptr1);
	const uint8_t* u8Ptr2 = reinterpret_cast<const uint8_t*>(ptr2);

	for(size_t i = 0; i < num; i++)	if(u8Ptr1[i] != u8Ptr2[i]) return (int)u8Ptr1[i] - (int)u8Ptr2[i];
	
	return 0;
}

extern "C" void* memset(void* ptr, uint8_t val, size_t num)
{
	uint8_t* u8PtrT = reinterpret_cast<uint8_t*>(ptr);
	while((uintptr_t)u8PtrT % 8 && num) {
		*u8PtrT++ = val;
		num--;
	}
	uint64_t* u64Ptr = reinterpret_cast<uint64_t*>(u8PtrT);
	size_t quot = num / sizeof(uint64_t);
	uint64_t u64Val = (uint64_t)val * 0x0101010101010101ULL;
	for(size_t i = 0; i < quot; i++) u64Ptr[i] = u64Val;
	uint8_t rem = num % sizeof(uint64_t);
	uint8_t* u8Ptr = reinterpret_cast<uint8_t*>(u64Ptr + quot);
	for(uint8_t i = 0; i < rem; i++) u8Ptr[i] = val;
	return ptr;
}

extern "C" void* memcpy(void* dst, const void* src, size_t num)
{
	uint8_t* u8DstT = reinterpret_cast<uint8_t*>(dst);
	const uint8_t* u8SrcT = reinterpret_cast<const uint8_t*>(src);

	while((uintptr_t)u8DstT % 8 && num) {
		*u8DstT++ = *u8SrcT++;
		num--;
	}
	
	uint64_t* u64Dst = reinterpret_cast<uint64_t*>(u8DstT);
	const uint64_t* u64Src = reinterpret_cast<const uint64_t*>(u8SrcT);

	size_t quot = num / sizeof(uint64_t);
	for(size_t i = 0; i < quot; i++) u64Dst[i] = u64Src[i];
	uint8_t rem = num % sizeof(uint64_t);
	uint8_t* u8Dst = reinterpret_cast<uint8_t*>(u64Dst + quot);
	const uint8_t* u8Src = reinterpret_cast<const uint8_t*>(u64Src + quot);
	for(uint8_t i = 0; i < rem; i++) u8Dst[i] = u8Src[i];
	return dst;
}