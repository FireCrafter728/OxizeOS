// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <defs.hpp>

#include <stddef.hpp>

#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>

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
			size_t GetFileSize(EFI_FILE_PROTOCOL* file);
			size_t GetFilePosition(EFI_FILE_PROTOCOL* file);
			constexpr EFI_STATUS GetLastStatus() { return lastStatus; }
		private:
			EFI_STATUS lastStatus;
		};
	}
}