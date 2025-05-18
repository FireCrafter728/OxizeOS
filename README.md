# OxizeOS
An OS Operating in BIOS / CSM. Currently only bootable from qemu

# Installation process
1. Installing dependencies
2. Creating toolchains
3. Building the OS From source
4. Running the OS

## 1) Installing dependencies
- These dependencies are required for the toolchain build process: `wget` `tar` `build-essential` `libgmp-dev` `libmpfr-dev` `libmpc-dev` `libisl-dev` `texinfo`
- These dependencies are required for the OS build process: `build-essential` `nasm` `dosfstools`
- Optionally, **VSCode** for developement

## 2) Creating toolchains
- To create the toolchains, run `make toolchain`. Currently, the GCC/Binutils versions are 13.2.0 & 2.38, if the build process fails, try using older versions

## 3) Building the OS from source
- To build the OS, run `make` or `make -s` for clean output

## 4) Running the OS
- To run the OS, use `./run` in the terminal
- To debug the OS using GDB, use `./debug` in the terminal

# Note
- This OS Is only avaiable in BIOS with CSM support, UEFI support is not planned for now
- This OS Is currently under developement, things may break, and the build process isn't fully tested
- This OS Is Open-Sourced, meaning you can customize it as much as you want