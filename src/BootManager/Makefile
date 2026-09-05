# SPDX-License-Identifier: GPL-3.0-or-later
# OxizeOS build system

X64_TARGET_LINKFLAGS += -T linker.ld -Wl,--subsystem,10 -Wl,--image-base,0 -e efi_main -Wl,--Map=$(OUTPUT)/BootMgr.map
X64_TARGET_CXXFLAGS += -fno-exceptions -fno-rtti -fno-stack-protector -mno-red-zone \
					   -fno-asynchronous-unwind-tables  -fno-use-cxa-atexit \
					   -mno-avx -mno-avx2 -mno-avx512f -mno-avx512bw -mno-avx512dq -mno-avx512vl -mno-fma \
					   -msse2 -fshort-wchar -MMD -MP\
					   -I Include/ -I Include/std/ -I $(LIB)/EDK2/MdePkg/Include  -I $(LIB)/OxizeOS/

SOURCES_CPP = $(shell find Source -name '*.cpp')
SOURCES_ASM = $(shell find Source -name '*.asm')

OBJECTS_CPP = $(patsubst Source/%.cpp, $(OBJ)/BootManager/cpp/%.obj, $(SOURCES_CPP))
OBJECTS_ASM = $(patsubst Source/%.asm, $(OBJ)/BootManager/asm/%.obj, $(SOURCES_ASM))

build: dir $(OUTPUT)/BootMgr.exe

$(OUTPUT)/BootMgr.exe: $(OBJECTS_CPP) $(OBJECTS_ASM) linker.ld
	$(X64_TARGET_LDXX) $(X64_TARGET_LINKFLAGS) -o $@ $(OBJECTS_CPP) $(OBJECTS_ASM)
	printf "\033[1m\033[33m[BOOTMGR]    \033[92mCreated -> $@\033[0m\n"

$(OBJ)/BootManager/cpp/%.obj: Source/%.cpp
	mkdir -p $(@D)
	$(X64_TARGET_CXX) $(X64_TARGET_CXXFLAGS) -c -o $@ $<
	printf "\033[1m\033[33m[BOOTMGR]    \033[96mCompiled -> $<\033[0m\n"

$(OBJ)/BootManager/asm/%.obj: Source/%.asm
	mkdir -p $(@D)
	$(X64_TARGET_ASM) $(X64_TARGET_ASMFLAGS) -o $@ $<
	printf "\033[1m\033[33m[BOOTMGR]    \033[91mAssembled -> $<\033[0m\n"

dir:
	mkdir -p $(OBJ)/BootManager/cpp
	mkdir -p $(OBJ)/BootManager/asm

-include $(OBJECTS_CPP:.obj=.d)