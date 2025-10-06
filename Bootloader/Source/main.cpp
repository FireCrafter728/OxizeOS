#include <Uefi.h>

extern "C"
EFI_STATUS
EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* System)
{
	System->ConOut->ClearScreen(System->ConOut);
	System->ConOut->OutputString(System->ConOut, (CHAR16*)L"Hello, World from OxizeOS in UEFI!\r\n");

	EFI_INPUT_KEY Key;
	while (System->ConIn->ReadKeyStroke(System->ConIn, &Key) != EFI_SUCCESS);
	
	System->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

	return EFI_SUCCESS;
}