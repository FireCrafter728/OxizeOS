# SPDX-License-Identifier: GPL-3.0-or-later
# OxizeOS build system

X64_TARGET_LINKFLAGS += -T linker.ld
X64_TARGET_CXXFLAGS += -fno-exceptions -fno-rtti -fshort-wchar -mno-red-zone -fno-stack-protector -Wno-address-of-packed-member -fno-threadsafe-statics -I Include/ -I Include/std/ -include Global.hpp -I $(LIB)/OxizeOS/ -g -O0
X64_TARGET_ASMFLAGS += -f elf64 -g -F dwarf

SOURCES_CPP = $(shell find Source -name '*.cpp')
SOURCES_ASM = $(shell find Source -name '*.asm')

OBJECTS_CPP = $(patsubst Source/%.cpp, $(OBJ)/syskrnl64/cpp/%.obj, $(SOURCES_CPP))
OBJECTS_ASM = $(patsubst Source/%.asm, $(OBJ)/syskrnl64/asm/%.obj, $(SOURCES_ASM))

build: dir gen_isrs $(OUTPUT)/syskrnl64.exe

$(OUTPUT)/syskrnl64.exe: $(OBJECTS_CPP) $(OBJECTS_ASM)
	$(X64_TARGET_LDXX) $(X64_TARGET_LINKFLAGS) -o $@ $^ $(X64_TARGET_LIBS)
	printf "\033[1m\033[33m[SysKrnl64]  \033[92mCreated -> $@\033[0m\n"

$(OBJ)/syskrnl64/cpp/%.obj: Source/%.cpp
	mkdir -p $(@D)
	$(X64_TARGET_CXX) $(X64_TARGET_CXXFLAGS) -c -o $@ $<
	printf "\033[1m\033[33m[SysKrnl64]  \033[96mCompiled -> $<\033[0m\n"

$(OBJ)/syskrnl64/asm/%.obj: Source/%.asm
	mkdir -p $(@D)
	$(X64_TARGET_ASM) $(X64_TARGET_ASMFLAGS) -o $@ $<
	printf "\033[1m\033[33m[SysKrnl64]  \033[91mAssembled -> $<\033[0m\n"

dir:
	mkdir -p $(OBJ)/syskrnl64/cpp
	mkdir -p $(OBJ)/syskrnl64/asm

ISR_GEN_CPP = $(SRC)/syskrnl64/Source/Drivers/Interrupts/gen_isrs.cpp
ISR_GEN_ASM = $(SRC)/syskrnl64/Include/Drivers/Interrupts/gen_isrs.inc
ISR_GEN_SCRIPT = $(SCRIPTS)/gen_isrs.py

gen_isrs: $(ISR_GEN_CPP) $(ISR_GEN_ASM)

$(ISR_GEN_CPP) $(ISR_GEN_ASM): $(ISR_GEN_SCRIPT)
	python $(ISR_GEN_SCRIPT) $(ISR_GEN_CPP) $(ISR_GEN_ASM)