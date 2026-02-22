EFI_GUID gEfiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
EFI_GUID gEfiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_GUID gEfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
EFI_GUID gEfiAcpi20TableGuid = EFI_ACPI_20_TABLE_GUID;
EFI_GUID gEfiSmbios3TableGuid = SMBIOS3_TABLE_GUID;
EFI_GUID gEfiSmbiosTableGuid = SMBIOS_TABLE_GUID;

bool BootMgr::CompareGUID(EFI_GUID guid1, EFI_GUID guid2)
{
	if(memcmp((void*)&guid1, (void*)&guid2, sizeof(EFI_GUID)) == 0) return true;

	return false;
}