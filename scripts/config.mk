MAKE_DISK_SIZE = 134217728 # 128 MiB, 134.22 MB

export CFLAGS = -std=c17 -Wall -Wextra -O2
export CXXFLAGS = -std=c++17 -Wall -Wextra -O2
export ASMFLAGS =
export CC = gcc
export CXX = g++
export LD = gcc
export LDXX = g++
export AR = ar
export ASM = nasm
export LINKFLAGS =
export ARFLAGS =
export LIBS =


export TARGET = i686-elf
export TARGET_ASM = nasm
export TARGET_ASMFLAGS =
export TARGET_CFLAGS = -std=c11 -O2 -Wall -Wextra
export TARGET_CXXFLAGS = -std=c++11 -O2 -Wall -Wextra
export TARGET_CC = $(TARGET)-gcc
export TARGET_CXX = $(TARGET)-g++
export TARGET_LD = $(TARGET)-gcc
export TARGET_LDXX = $(TARGET)-g++
export TARGET_AR = $(TARGET)-ar
export TARGET_LINKFLAGS =
export TARGET_ARFLAGS =
export TARGET_LIBS =

export output = $(abspath output)
export scripts = $(abspath scripts)
export src = $(abspath src)
export lib = $(abspath lib)

BINUTILS_VERSION = 2.38
BINUTILS_URL = https://ftp.gnu.org/gnu/binutils/binutils-$(BINUTILS_VERSION).tar.xz

GCC_VERSION = 13.2.0
GCC_URL = https://ftp.gnu.org/gnu/gcc/gcc-$(GCC_VERSION)/gcc-$(GCC_VERSION).tar.xz