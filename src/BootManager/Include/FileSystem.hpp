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

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |---------------------------------------------------------------------------| //
// | OxizeOS Boot Manager Implementation                                       | //
// | FileSystem: An abstraction layer for the UEFI Simple File System protocol | //
// |---------------------------------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include <stdint.hpp>

namespace BootMgr
{
	namespace FS
	{
		class FS
		{
		public:
			EFI_HANDLE GetVolumeHandle(EFI_HANDLE ImageHandle);
			EFI_FILE_PROTOCOL* OpenVolume(EFI_HANDLE VolumeHandle);
			EFI_FILE_PROTOCOL* OpenFile(EFI_FILE_PROTOCOL* volume, const wchar_t* path, uint64_t openMode = EFI_FILE_MODE_READ, uint64_t attribs = 0);
			size_t ReadFile(EFI_FILE_PROTOCOL* file, void* buffer, size_t length);
			EFI_STATUS Seek(EFI_FILE_PROTOCOL* file, size_t offset);
			constexpr EFI_STATUS GetLastStatus() { return lastStatus; }
		private:
			EFI_STATUS lastStatus;
		};
	}
}