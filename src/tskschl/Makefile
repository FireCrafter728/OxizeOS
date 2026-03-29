X64_TARGET_LINKFLAGS += -T linker.ld
X64_TARGET_CXXFLAGS += -fno-exceptions -fno-rtti -fshort-wchar -mno-red-zone -fno-stack-protector -Wno-address-of-packed-member -I Include/ -I Include/std/ -include Global.hpp -I $(LIB)/OxizeOS/
X64_TARGET_ASMFLAGS += -f elf64

SOURCES_CPP = $(shell find Source -name '*.cpp')
SOURCES_ASM = $(shell find Source -name '*.asm')

OBJECTS_CPP = $(patsubst Source/%.cpp, $(OBJ)/tskschl/cpp/%.obj, $(SOURCES_CPP))
OBJECTS_ASM = $(patsubst Source/%.asm, $(OBJ)/tskschl/asm/%.obj, $(SOURCES_ASM))

build: dir gen_isrs $(OUTPUT)/tskschl.exe

$(OUTPUT)/tskschl.exe: $(OBJECTS_CPP) $(OBJECTS_ASM)
	$(X64_TARGET_LDXX) $(X64_TARGET_LINKFLAGS) -o $@ $^
	printf "\033[31mCreated -> $@\033[0m\n"

$(OBJ)/tskschl/cpp/%.obj: Source/%.cpp
	mkdir -p $(@D)
	$(X64_TARGET_CXX) $(X64_TARGET_CXXFLAGS) -c -o $@ $<
	printf "\033[31mCompiled -> $<\033[0m\n"

$(OBJ)/tskschl/asm/%.obj: Source/%.asm
	mkdir -p $(@D)
	$(X64_TARGET_ASM) $(X64_TARGET_ASMFLAGS) -o $@ $<
	printf "\033[31mAssembled -> $<\033[0m\n"

dir:
	mkdir -p $(OBJ)/tskschl/cpp
	mkdir -p $(OBJ)/tskschl/asm

ISR_GEN_CPP = $(SRC)/tskschl/Source/Drivers/Interrupts/gen_isrs.cpp
ISR_GEN_ASM = $(SRC)/tskschl/Include/Drivers/Interrupts/gen_isrs.inc
ISR_GEN_SCRIPT = $(SCRIPTS)/gen_isrs.py

gen_isrs: $(ISR_GEN_CPP) $(ISR_GEN_ASM)

$(ISR_GEN_CPP) $(ISR_GEN_ASM): $(ISR_GEN_SCRIPT)
	python $(ISR_GEN_SCRIPT) $(ISR_GEN_CPP) $(ISR_GEN_ASM)