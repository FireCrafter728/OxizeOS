# OxizeOS
An OS Operating in BIOS / CSM. Currently supported on most emulators, but might have problems with qemu / bochs

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
- Currently, both **QEMU** & **VirtualBox** are supported for testing, to use VBox, you need to install it seperately, as long as it is in the system path
- If you're using qemu, uncomment the qemu-system-x86_64 line and comment everything else in `./run`

## 3) Building the OS from source
- To build the OS, run `make` or `make -s` for clean output

## 4) Running the OS
- To run the OS, use `./run` in the terminal
- Debugging is currently not supported

# Note
- This OS Is only avaiable in BIOS with CSM support, UEFI support is not planned for now
- This OS Is currently under developement, things may break, and the build process isn't fully tested
- This OS Is Open-Sourced, meaning you can customize it as much as you want
- QEMU installation is broken for me, so the updates will be relevant to VBox
- x86Kernel is currently operating in both C & C++, things may break