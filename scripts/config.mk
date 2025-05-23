MAKE_DISK_SIZE = 134217728 # 128 MiB, 134.22 MB

export CFLAGS = -std=c17 -Wall -Wextra -O2
export ASMFLAGS =
export CC = gcc
export CXX = g++
export LD = gcc
export LDXX = g++
export ASM = nasm
export LINKFLAGS =
export LIBS =

export TARGET = i686-elf
export TARGET_ASM = nasm
export TARGET_ASMFLAGS =
export TARGET_CFLAGS = -std=c99 -O2
export TARGET_CC = $(TARGET)-gcc
export TARGET_CXX = $(TARGET)-g++
export TARGET_LD = $(TARGET)-gcc
export TARGET_LDXX = $(TARGET)-g++
export TARGET_LINKFLAGS =
export TARGET_LIBS =

export output = $(abspath output)
export scripts = $(abspath scripts)
export src = $(abspath src)

BINUTILS_VERSION = 2.38
BINUTILS_URL = https://ftp.gnu.org/gnu/binutils/binutils-$(BINUTILS_VERSION).tar.xz

GCC_VERSION = 13.2.0
GCC_URL = https://ftp.gnu.org/gnu/gcc/gcc-$(GCC_VERSION)/gcc-$(GCC_VERSION).tar.xz