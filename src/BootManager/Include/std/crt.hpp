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
// | CRT: C/C++ runtime function implementations                                | //
// |----------------------------------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

extern "C"
{
	typedef void (*ctor_t)();

	extern ctor_t __init_array_start[];
	extern ctor_t __init_array_end[];

	void __cxa_init_global_ctors();
}

