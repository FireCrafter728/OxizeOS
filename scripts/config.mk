export OUTPUT=$(abspath output)
export OBJ=$(abspath obj)
export SRC=$(abspath src)
export TOOLS=$(abspath tools)
export LIB=$(abspath lib)
export TOOLCHAIN=$(abspath toolchain)
export SCRIPTS=$(abspath scripts)

export CARGO=cargo
export CARGOFLAGS=build --release

export CC=gcc
export CXX=g++
export LD=gcc
export LDXX=g++
export AR=ar
export ASM=nasm
export CFLAGS= -std=c23 -Wall -Wextra -O2 -m64
export CXXFLAGS= -std=c++23 -Wall -Wextra -O2 -m64
export ASMFLAGS=
export LINKFLAGS=
export ARFLAGS
export LIBS=

export X64_TARGET=x86_64-elf
export X64_TARGET_CC=$(X64_TARGET)-gcc
export X64_TARGET_CXX=$(X64_TARGET)-g++
export X64_TARGET_LD=$(X64_TARGET)-gcc
export X64_TARGET_LDXX=$(X64_TARGET)-g++
export X64_TARGET_AR=$(X64_TARGET)-ar
export X64_TARGET_ASM=nasm
export X64_TARGET_CFLAGS= -std=c23 -Wall -Wextra -O2 -ffreestanding -nostdlib -nostdinc -fPIC -m64
export X64_TARGET_CXXFLAGS= -std=c++23 -Wall -Wextra -O2 -ffreestanding -nostdlib -nostdinc -fPIC -m64
export X64_TARGET_ASMFLAGS=
export X64_TARGET_LINKFLAGS=-nostdlib -pie
export X64_TARGET_ARFLAGS=
export X64_TARGET_LIBS=-lgcc

export I686_TARGET=i686-elf
export I686_TARGET_CC=$(I686_TARGET)-gcc
export I686_TARGET_CXX=$(I686_TARGET)-g++
export I686_TARGET_LD=$(I686_TARGET)-gcc
export I686_TARGET_LDXX=$(I686_TARGET)-g++
export I686_TARGET_AR=$(I686_TARGET)-ar
export I686_TARGET_ASM=nasm
export I686_TARGET_CFLAGS= -std=c23 -Wall -Wextra -O2 -ffreestanding -nostdlib -nostdinc -fPIC -m32
export I686_TARGET_CXXFLAGS= -std=c++23 -Wall -Wextra -O2 -ffreestanding -nostdlib -nostdinc -nostdinc++ -fPIC -m32
export I686_TARGET_ASMFLAGS=
export I686_TARGET_LINKFLAGS=-nostdlib -pie
export I686_TARGET_ARFLAGS=
export I686_TARGET_LIBS=-lgcc

export MINGW_CXX=x86_64-w64-mingw32-g++
export MINGW_LDXX=x86_64-w64-mingw32-g++
export MINGW_CXXFLAGS= -std=c++23 -Wall -Wextra -O2 -ffreestanding -nostdlib -nostdinc -nostdinc++ -m64 
export MINGW_LINKFLAGS= -nostdlib

export GCC_VERSION=15.2.0
export BINUTILS_VERSION=2.46.0

export GCC_URL=https://ftp.gnu.org/gnu/gcc/gcc-$(GCC_VERSION)/gcc-$(GCC_VERSION).tar.gz
export BINUTILS_URL=https://ftp.gnu.org/gnu/binutils/binutils-$(BINUTILS_VERSION).tar.gz

export NPROC=$(shell nproc)