# Changelog for OxizeOS
### <date> <version> <Name> <build type>

## 2025-10-06 0.1.0001 Initial build InDev unconfirmed
- Has a minimal bootx64.efi that prints a message and shuts down the system upon a keypress

## 2025-10-08 0.1.0002 Utility build InDev unconfirmed
- Adds `Print`, `PrintW`, `PrintChr`, `PrintWChr`, `printf`, `vprintf`; macros: `puts`, `putc`, `putwc`, `putsw`, `printf`
- Introduces `new`, `new[]`, `delete`, `delete[]`; Adds `MemoryAlloc` & `MemoryFree` that use `AllocatePool` & `FreePool` from `EFI_SYSTEM_TABLE`
- Adds `HaltSystem`, which prints a message "System halted" and calls `HaltSystemImpl`, which calls instructions `CLI` & `HLT`, which make the bootloader unresponsive
- Adds `AsciiToUnicode`, which converts ASCII strings into Unicode strings, using a 16KiB buffer, meaning 8191 total characters.
- And some other changes

## 2025-10-08 0.1.0003 PCI Implementation build InDev unconfirmed
- Added implementation for the PCI driver for communicating with PCI devices
- Also adds `inb`, `outb`, `inw`, `outw`, `ind`, `outd`