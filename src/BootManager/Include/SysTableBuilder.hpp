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

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |-------------------------------------------------------------------------------------------------------------------------------------------------------------| //
// | OxizeOS Boot Manager Implementation                                                                                                                         | //
// | SysTableBuilder: A driver that collects information about the system and creates various structures for the kernel and returns a descriptor describing them | //
// |-------------------------------------------------------------------------------------------------------------------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <SysTable.hpp>

namespace BootMgr
{
	extern EFI_SYSTEM_TABLE* gSystem;
	namespace SysTable
	{
		class SysTable
		{
		public:
			SystemTable* BuildSystemTable(size_t SysKrnl64PageCount, EFI_SYSTEM_TABLE* sysTable = gSystem);
			constexpr EFI_STATUS GetLastStatus() { return lastStatus; }
			constexpr void* getEfiGOP() { return gop; }
			constexpr uintptr_t getPageTablesPhysAddr() { return PageTablesPhys; }
			constexpr uintptr_t getRegionStartAddr() { return regionStart; }
		private:
			EFI_STATUS lastStatus;
			EFI_MEMORY_DESCRIPTOR* efiMemmap;
			uint64_t efiMemmapEntryCount;
			uint64_t entrySize;
			size_t allocSize;
			void* gop;
			uintptr_t PageTablesPhys;
			uintptr_t regionStart;
		};
	}
}