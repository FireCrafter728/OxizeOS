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

#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |----------------------------------------------------------------------------| //
// | Minimal freestanding LIBSTDC++ Implementation for the OxizeOS boot manager | //
// | STDARG: variadic argument management types and functions definitions       | //
// |----------------------------------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

typedef __builtin_va_list va_list;

#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap)        __builtin_va_end(ap)
#define va_arg(ap, type)  __builtin_va_arg(ap, type)
#define va_copy(dest, src) __builtin_va_copy(dest, src)