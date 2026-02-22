#pragma once

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