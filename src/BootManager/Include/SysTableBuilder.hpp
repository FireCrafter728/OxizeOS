// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <defs.hpp>
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