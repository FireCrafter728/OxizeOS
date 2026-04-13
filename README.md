# OxizeOS

## Sections

- [About](#about)
- [Building](#building)
- [Dependencies](#dependencies)
- [Running the OS](#running-the-os)
- [Others](#others)

## About

**OxizeOS** is an OS that is currently under developement, running in `UEFI`
and uses it's own UEFI bootloader, currently very minimal
Currently, **OxizeOS** is in the very early stages of developement, and has recently
been completely restarted, abandoning the old **32-bit** Attempt, and targeting **64-bit**
long mode

## Building

1. You need to make sure you have all the dependencies installed on your system.
To make sure you have them installed, view the **Dependencies** section of this README.
2. Then you need to build the `gcc 15.2.0` & `binutils 2.46.0` toolchains. to do that,
run `make toolchain`.
3. Then you need to build the internal parts & build the image. for that, run `make`,
and optionally the `-s` flag for cleaner output. To run a multi-threaded build, run `./build`
4. Then you need to run `./run` to test the OS(or `./run qemu` to test with **QEMU** instead of **VirtualBox**). For that, you need to make sure you have
**Virtual Box** or **qemu** installed with it's tools avaiable in the system `PATH`.

## Dependencies

This OS depends on a lot of things. here are they, grouped by their use:

### Building the toolchain

For this you need:

- Working `GCC` & `Binutils` with full **C++14** support(I recommend `GCC 9.1+`)
- libraries `libgmp-dev`, `libmpfr-dev`, `libmpc-dev`, `libisl-dev`
- tools `flex`, `bison`, `gawk`, `make`, `tar`, `wget`

### Building the OS

For this you need:

- To make sure the toolchain is installed correctly & working
- Working base `GCC` & `Binutils` with **C++23** support (I recommend `GCC 14/15` or later)
- `cargo` installed together with `rustc`
- tools `gdisk`, `dd`, `nasm`, `mingw-w64`
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
on real hardware on poorly/not tested at all snapshots/releases. Use it at your own risk.
We recommend using it on a **VM**, like **qemu**, **Virtual Box** or **VMware**.
- This OS is running on **UEFI** and needs a chipset that has **PCIe** support(like **ICH9**),
**PCI** is not supported
- This OS might have a **lack of drivers**, since they are **community-developed**,
not by the manufacturers.
