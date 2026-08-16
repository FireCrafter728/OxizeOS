# SPDX-License-Identifier: GPL-3.0-or-later
# OxizeOS build configuration

export OUTPUT=$(abspath output)
export OBJ=$(abspath obj)
export SRC=$(abspath src)
export TOOLS=$(abspath tools)
export LIB=$(abspath lib)
export SCRIPTS=$(abspath scripts)

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

export X64_TARGET=x86_64-w64-mingw32
export X64_TARGET_CC=$(X64_TARGET)-gcc
export X64_TARGET_CXX=$(X64_TARGET)-g++
export X64_TARGET_LD=$(X64_TARGET)-gcc
export X64_TARGET_LDXX=$(X64_TARGET)-g++
export X64_TARGET_AR=$(X64_TARGET)-ar
export X64_TARGET_WINDRES=$(X64_TARGET)-windres
export X64_TARGET_ASM=nasm

export X64_TARGET_CFLAGS= -std=c23 -Wall -Wextra -O2 -ffreestanding -nostdlib -nostdinc -m64
export X64_TARGET_CXXFLAGS= -std=c++23 -Wall -Wextra -O2 -ffreestanding -nostdlib -nostdinc -nostdinc++ -m64
export X64_TARGET_LINKFLAGS=-nostdlib
export X64_TARGET_LIBS=-lgcc
export X64_TARGET_ARFLAGS=
export X64_TARGET_ASMFLAGS= -f win64
export X64_TARGET_WINDRES_FLAGS=