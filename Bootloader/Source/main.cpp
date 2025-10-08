#include <print.hpp>
print Print;

EFI_SYSTEM_TABLE* gSystem;

extern "C"
EFI_STATUS
EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* System)
{
	gSystem = System;
	Print.SetStdout(System->ConOut);
	Print.ClrScr();
	printf("OxizeOS Bootloader V0.1.0002");

	EFI_INPUT_KEY Key;
	while (System->ConIn->ReadKeyStroke(System->ConIn, &Key) != EFI_SUCCESS);
	
	System->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

	return EFI_SUCCESS;
}