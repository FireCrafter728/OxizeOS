// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |--------------------------------------------------------------------| //
// | Minimal freestanding LIBSTDC Implementation for the OxizeOS kernel | //
// | STDARG: various types and definitions for variadic arguments       | //
// |--------------------------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

// ------- //
// VA_LIST //
// ------- //

typedef __builtin_va_list va_list;

#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap)        __builtin_va_end(ap)
#define va_arg(ap, type)  __builtin_va_arg(ap, type)
#define va_copy(dest, src) __builtin_va_copy(dest, src)