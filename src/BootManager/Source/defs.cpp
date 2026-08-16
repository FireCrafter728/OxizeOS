// SPDX-License-Identifier: GPL-3.0-or-later

#include <defs.hpp>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/Smbios.h>
#include <Guid/Acpi.h>
#include <Guid/SmBios.h>
#include <Guid/FileInfo.h>

#include <string.hpp>

EFI_GUID gEfiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
EFI_GUID gEfiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_GUID gEfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
EFI_GUID gEfiAcpi20TableGuid = EFI_ACPI_20_TABLE_GUID;
EFI_GUID gEfiSmbios3TableGuid = SMBIOS3_TABLE_GUID;
EFI_GUID gEfiSmbiosTableGuid = SMBIOS_TABLE_GUID;
EFI_GUID gEfiFileInfoGuid = EFI_FILE_INFO_ID;

bool BootMgr::CompareGUID(EFI_GUID guid1, EFI_GUID guid2)
{
	if(memcmp((void*)&guid1, (void*)&guid2, sizeof(EFI_GUID)) == 0) return true;

	return false;
}