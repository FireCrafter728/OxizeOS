# OxizeOS

# Sections
- [About](#about)
- [Building](#building)
- [Dependencies](#dependencies)
- [Others](#others)

## About
**OxizeOS** is an OS that is currently under developement, running in `UEFI`
and uses the `Limine` bootloader with the **limine protocol** to load **OxizeOS** kernel.
Currently, **OxizeOS** is in the very early stages of developement, and has recently
been completely restarted, abandoning the old, `MSVC` Bootloader for `UEFI` due to the
problems with UEFI.

## Building
1. You need to make sure you have all the dependencies installed on your system.
To make sure you have them installed, view the **Dependencies** section of this readme.
2. Then you need to build the `gcc 15.2.0` & `binutils 2.45` toolchains. to do that, 
run `make toolchain`.
3. Then you need to build the internal parts & build the image. for that, run `make`,
and optionally the `-s` flag for cleaner output.
4. Then you run `./run` to test the OS. For that, you need to make sure you have
**Virtual Box** installed with it's tools avaiable in the system `PATH`.

## Dependencies
This OS depends on a lot of things. here are they, grouped by their use:

### Building the toolchain
For this you need:
- Working `GCC` & `Binutils` with full **C++14** support(I recommend `GCC 9.1+`)
- libraries `libgmp-dev`, `libmpfr-dev`, `libmpc-dev`, `libisl-dev`
- tools `flex`, `bison`, `gawk`, `make`, optionally `texinfo`
### Building the OS
For this you need:
- To make sure the toolchain is installed correctly & working
- Working base `GCC` & `Binutils` with **C++17** support (I recommend `GCC 9.1+`)
- `cargo` installed together with `rustc`
- tools `gdisk` & `dd`

## Others
- This OS is currently in the **Early-Developement** stages, might contain bugs and is
subject to change. **It might crash, brick your system or damage internal components**
on real hardware if not tested enough or poorly/badly. Use it at your own risk.
We recommend using it on a **VM**.
- This OS is running on **UEFI** and requires a **Chipset** that has **PCIe** support(like **ICH9**).
Most modern motherboards will support **PCIe**.
- This OS might have a **lack of drivers**, since they are **community-developed**,
not by the manufacturers.
