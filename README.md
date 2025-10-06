# OxizeOS

## About

**OxizeOS** is an OS operating in UEFI, and is currently under developement
Current **OxizeOS** version is **v0.1.0001**(major.minor.build)

## How to compile and use

- To compile the `bootx64.efi`, open the `OxizeOS.sln`, and use **Ctrl + shift + B** in Visual Studio 2022 to build 1bootx64.efi1
- Currently, there is no automated script for writing a disk image because of lack of software, but there are plans to create a tool in C++ that fully handles that. You have to setup your own image / disk with an ESP, and put the `bootx64.efi` into **EFI/BOOT/** for the OS to work.
- The OS Can work on real hardware, but depends on firmware & it's settings

## Extra

- This is a remaster of **OxizeOS**(Deprecated BIOS version), will have different file structure, but the rough look should be similar.