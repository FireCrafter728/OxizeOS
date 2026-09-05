# SPDX-License-Identifier: GPL-3.0-or-later
# OxizeOS build system

# Extra build flags

# Specify full OS version for the kernel by combining OS and image version fields to get the 4-number format
# For the subsystem, use our own custom one
# 0x140 -> Kernel-level executable, either the kernel itself or a kernel driver
X64_TARGET_LINKFLAGS += -T linker.ld -Wl,-Map=$(OUTPUT)/syskrnl64.map,--image-base,0 \
						-Wl,--major-os-version,1,--minor-os-version,0,--major-image-version,1,--minor-image-version,14 \
						-Wl,--subsystem,0x140,--major-subsystem-version,1,--minor-subsystem-version,0 -flto

X64_TARGET_CXXFLAGS += -fno-exceptions -fno-rtti -mno-red-zone -fno-stack-protector \
					   -Wno-address-of-packed-member -fno-threadsafe-statics -fno-asynchronous-unwind-tables -fno-use-cxa-atexit \
					   -mno-avx -mno-avx2 -mno-avx512f -mno-avx512bw -mno-avx512dq -mno-avx512vl -mno-fma \
					   -msse2 -fshort-wchar -MMD -MP -O0 -g\
					   -I Include/ -I Include/std/ -I $(LIB)/OxizeOS/
X64_TARGET_WINDRES_FLAGS += -O coff

# Collect sources and format output object names

SOURCES_CPP = $(shell find Source -name '*.cpp')
SOURCES_ASM = $(shell find Source -name '*.asm')

OBJECTS_CPP = $(patsubst Source/%.cpp, $(OBJ)/syskrnl64/cpp/%.obj, $(SOURCES_CPP))
OBJECTS_ASM = $(patsubst Source/%.asm, $(OBJ)/syskrnl64/asm/%.obj, $(SOURCES_ASM))

RESOURCE_SCRIPTS = $(shell find . -name '*.rc')
RESOURCE_OBJECTS = $(patsubst %.rc, $(OBJ)/syskrnl64/res/%.res.obj, $(RESOURCE_SCRIPTS))

# Default target

build: dir gen_isrs $(OUTPUT)/syskrnl64.exe

# Linking step

$(OUTPUT)/syskrnl64.exe: $(OBJECTS_CPP) $(OBJECTS_ASM) $(RESOURCE_OBJECTS) linker.ld
	$(X64_TARGET_LDXX) $(X64_TARGET_LINKFLAGS) -o $@ $(OBJECTS_CPP) $(OBJECTS_ASM) $(RESOURCE_OBJECTS) $(X64_TARGET_LIBS)
	printf "\033[1m\033[33m[SysKrnl64]  \033[92mCreated -> $@\033[0m\n"

# C++ compiling step

$(OBJ)/syskrnl64/cpp/%.obj: Source/%.cpp
	mkdir -p $(@D)
	$(X64_TARGET_CXX) $(X64_TARGET_CXXFLAGS) -c -o $@ $<
	printf "\033[1m\033[33m[SysKrnl64]  \033[96mCompiled -> $<\033[0m\n"

# Assembling step

$(OBJ)/syskrnl64/asm/%.obj: Source/%.asm
	mkdir -p $(@D)
	$(X64_TARGET_ASM) $(X64_TARGET_ASMFLAGS) -o $@ $<
	printf "\033[1m\033[33m[SysKrnl64]  \033[91mAssembled -> $<\033[0m\n"

# Resource script compiling step

$(OBJ)/syskrnl64/res/%.res.obj: %.rc
	mkdir -p $(@D)
	$(X64_TARGET_WINDRES) $(X64_TARGET_WINDRES_FLAGS) -o $@ $<
	printf "\033[1m\033[33m[SysKrnl64]  \033[37mCompiled Resource Script -> $<\033[0m\n"

# Create directories before building

dir:
	mkdir -p $(OBJ)/syskrnl64/cpp
	mkdir -p $(OBJ)/syskrnl64/asm
	mkdir -p $(OBJ)/syskrnl64/res

# Generate source files

ISR_GEN_CPP = $(SRC)/syskrnl64/Source/arch/x86_64/Interrupts/gen_isrs.cpp
ISR_GEN_ASM = $(SRC)/syskrnl64/Include/arch/x86_64/Interrupts/gen_isrs.inc
ISR_GEN_SCRIPT = $(SCRIPTS)/gen_isrs.py

gen_isrs: $(ISR_GEN_CPP) $(ISR_GEN_ASM)

$(ISR_GEN_CPP) $(ISR_GEN_ASM): $(ISR_GEN_SCRIPT)
	python $(ISR_GEN_SCRIPT) $(ISR_GEN_CPP) $(ISR_GEN_ASM)

# Special include to activate dependency tracking

-include $(OBJECTS_CPP:.obj=.d)