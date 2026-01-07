export OUTPUT=$(abspath output)
export OBJ=$(abspath obj)
export SRC=$(abspath src)
export TOOLS=$(abspath tools)
export LIB=$(abspath lib)
export TOOLCHAIN=$(abspath toolchain)

export CARGO=cargo
export CARGOFLAGS=build --release

export CC=gcc
export CXX=g++
export LD=gcc
export LDXX=g++
export AR=ar
export ASM=nasm
export CFLAGS= -std=c17 -Wall -Wextra -O2
export CXXFLAGS= -std=c++17 -Wall -Wextra -O2
export ASMFLAGS=
export LINKFLAGS=
export ARFLAGS
export LIBS=


export TARGET=x86_64-elf
export TARGET_CC=$(TARGET)-gcc
export TARGET_CXX=$(TARGET)-g++
export TARGET_LD=$(TARGET)-gcc
export TARGET_LDXX=$(TARGET)-g++
export TARGET_AR=$(TARGET)-ar
export TARGET_ASM=nasm
export TARGET_CFLAGS= -std=c23 -Wall -Wextra -O2 -ffreestanding -nostdlib -fPIC
export TARGET_CXXFLAGS= -std=c++23 -Wall -Wextra -O2 -ffreestanding -nostdlib -fPIC
export TARGET_ASMFLAGS=
export TARGET_LINKFLAGS=-nostdlib -pie
export TARGET_ARFLAGS=
export TARGET_LIBS=-lgcc

export GCC_VERSION=15.2.0
export BINUTILS_VERSION=2.45

export GCC_URL=https://ftp.gnu.org/gnu/gcc/gcc-$(GCC_VERSION)/gcc-$(GCC_VERSION).tar.gz
export BINUTILS_URL=https://ftp.gnu.org/gnu/binutils/binutils-$(BINUTILS_VERSION).tar.gz