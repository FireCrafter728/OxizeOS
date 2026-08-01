# OxizeOS

## Sections

- [About](#about)
- [Building](#building)
- [Dependencies](#dependencies)
- [Running the OS](#running-the-os)
- [Others](#others)

## About

This project is licensed under the **GNU General Public License version 3.0**
**OxizeOS** is an OS that is currently under developement, running in `UEFI`
and uses it's own UEFI bootloader, for now very minimal
**OxizeOS** is in the very early stages of developement, currently working on
the kernel(SysKrnl64)

## Building

1. You need to make sure you have all the dependencies installed on your system.
To make sure you have them installed, view the **Dependencies** section of this README.
2. Then you need to build the `gcc 16.1.0` & `binutils 2.46.1` toolchains. to do that,
run `make toolchain`.
3. Then you need to build the internal parts & build the image. for that, run `make`,
and optionally the `-s` flag for cleaner output. To run a multi-threaded build, run `./build`
4. Then you need to run `./run` to test the OS(or `./run qemu` to test with **QEMU** instead of **VirtualBox**). For that, you need to make sure you have
**Virtual Box** or **qemu** installed with it's tools avaiable in the system `PATH`.

## Dependencies

This OS depends on a handful of things. here are they, grouped by their use:

### Building the toolchain

For this you need:

- Working `GCC` & `Binutils` with full **C++14** support(I recommend at least `GCC 9.1`, preferrably later)
- libraries `libgmp-dev`, `libmpfr-dev`, `libmpc-dev`, `libisl-dev`
- tools `flex`, `bison`, `gawk`, `make`, `tar`, `wget`

### Building the OS

For this you need:

- To make sure the toolchain is installed correctly
- Working base `GCC` & `Binutils` with **C++23** support (I recommend at least `GCC 14`, preferrably the latest version)
- tools `dosfstools`, `nasm`, `mingw-w64`
- `python3` installed for scripts

## Running the OS

You can run the OS Using any VM hypervisor you like, but for simplicity,
a `run` script is provided that supports both **qemu** & **Virtual Box**.

- To run with **qemu**, run `./run qemu`
- To run with **Virtual Box**, run `./run`

If you are using **Virtual Box**, some arguments can be provided:

- `create`: creates the VM
- `recreate`: deletes & creates the VM, used when the default VM configurations
in the script have changed
- `delete`: deletes the VM

## Others

- This OS is currently in the **Early-Developement** stages, might contain bugs and is
subject to change. **It might crash, brick your system or damage internal components**
on real hardware on poorly/not tested at all developement updates/releases. Use it at your own risk.
I recommend using it on a **VM**, like **qemu**, **Virtual Box** or **VMware**.
- This OS is running on **UEFI** and needs a chipset that has **PCIe** support,
**PCI** is not supported
- This OS might have a **lack of drivers**, since they mostly are **community-developed**,
not by the manufacturers.
- Some build tools are replaced with our own for extra performance, however they are
not tested extensively