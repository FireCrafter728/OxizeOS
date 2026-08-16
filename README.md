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
the kernel

## Building

1. Make sure you have all the dependencies installed on your system.
To make sure you have them installed, view the **Dependencies** section of this README.

2. Build the OS components & the raw image by running `make`,
and optionally the `-s` flag for cleaner output. To run a multi-threaded build, run `./build`

3. Run `./run` to test the OS(or `./run qemu` to test with **QEMU** instead of **VirtualBox**). Make sure you have
**Virtual Box** or **qemu** installed with it's tools avaiable in the system `PATH`.

## Dependencies

- Working `GCC` & `Binutils` with **C++23** support (I recommend at least `GCC 1.14.0`, preferrably the latest version)
- Working installation of mingw-w64 with **C++23** support (I recommend at least version `1.14.0`, preferrably the latest version)
- tools `dosfstools` and `nasm`
- `python3` for scripts

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

If you are using **qemu**, you can specify the debug flag to run without kvm, the CPU stopped before execution and a TCP connection exposed trough port :1234 for GDB to attach to

## Others

- This OS is currently in the **Early-Developement** stages, might contain bugs and is
subject to change. **It might crash, brick your system or damage internal components**
on real hardware on poorly/not tested at all developement updates/releases. Use it at your own risk.
I recommend using it on a **VM**, like **qemu**, **Virtual Box** or **VMware**.
- This OS is running on **UEFI** and needs a chipset that has **PCIe** support,
currently **PCI** is not supported
- This OS might have a lack of drivers, since they mostly are **community-developed**,
not by the manufacturers.
- Some build tools are replaced with our own for extra performance, however they are
not tested extensively