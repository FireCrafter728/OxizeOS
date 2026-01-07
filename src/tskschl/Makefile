TARGET_CXXFLAGS += -fno-PIE -fno-exceptions -fno-rtti -fshort-wchar -I Include -include Global.hpp -nostdinc -nostdinc++ -I $(LIB)/UEFI/CryptoPkg/Include -I $(LIB)/UEFI/MdeModulePkg/Include -I $(LIB)/UEFI/MdePkg/Include -I $(LIB)/UEFI/ShellPkg/Include
TARGET_CFLAGS += -fshort-wchar -I Include -nostdinc -I $(LIB)/UEFI/CryptoPkg/Include -I $(LIB)/UEFI/MdeModulePkg/Include -I $(LIB)/UEFI/MdePkg/Include -I $(LIB)/UEFI/ShellPkg/Include
TARGET_ASMFLAGS += -f elf64
TARGET_LINKFLAGS += -no-pie -T linker.ld

SOURCES_CPP=$(shell find Source -name '*.cpp')
SOURCES_C=$(shell find Source -name '*.c')
SOURCES_ASM=$(shell find Source -name '*.asm')

OBJECTS_CPP=$(patsubst Source/%.cpp, $(OBJ)/tskschl/cpp/%.obj, $(SOURCES_CPP))
OBJECTS_C=$(patsubst Source/%.c, $(OBJ)/tskschl/c/%.obj, $(SOURCES_C))
OBJECTS_ASM=$(patsubst Source/%.asm, $(OBJ)/tskschl/asm/%.obj, $(SOURCES_ASM))

build: dir $(OUTPUT)/tskschl.exe

$(OUTPUT)/tskschl.exe: $(OBJECTS_CPP) $(OBJECTS_C) $(OBJECTS_ASM)
	$(TARGET_LDXX) $(TARGET_LINKFLAGS) -o $@ $^ $(TARGET_LIBS)
	echo "\033[92mCreated -> $@\033[0m"

$(OBJ)/tskschl/cpp/%.obj: Source/%.cpp
	mkdir -p $(@D)
	$(TARGET_CXX) $(TARGET_CXXFLAGS) -c -o $@ $<
	echo "\033[36mCompiled -> $<\033[0m"

$(OBJ)/tskschl/c/%.obj: Source/%.c
	mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<
	echo "\033[36mCompiled -> $<\033[0m"

$(OBJ)/tskschl/asm/%.obj: Source/%.asm
	mkdir -p $(@D)
	$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<
	echo "\033[36mAssembled -> $<\033[0m"

dir:
	mkdir -p $(OBJ)/tskschl/cpp
	mkdir -p $(OBJ)/tskschl/c
	mkdir -p $(OBJ)/tskschl/asm