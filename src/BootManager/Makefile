MINGW_LINKFLAGS += -no-pie -T linker.ld -Wl,--subsystem,10 -Wl,--image-base,0 -e efi_main -Wl,--Map=$(OUTPUT)/BootMgr.map
MINGW_CXXFLAGS += -fno-PIC -fno-exceptions -fno-rtti -fno-stack-protector -fno-asynchronous-unwind-tables -mno-red-zone -fno-use-cxa-atexit -mno-avx -mno-avx2 -mno-avx512f -mno-avx512bw -mno-avx512dq -mno-avx512vl -mno-fma -msse2 -I Include/ -I Include/std/ -include Global.hpp -I $(LIB)/UEFI/CryptoPkg/Include -I $(LIB)/UEFI/MdeModulePkg/Include -I $(LIB)/UEFI/MdePkg/Include -I $(LIB)/UEFI/ShellPkg/Include -fshort-wchar -I $(LIB)/OxizeOS/
X64_TARGET_ASMFLAGS += -f win64

SOURCES_CPP = $(shell find Source -name '*.cpp')
SOURCES_ASM = $(shell find Source -name '*.asm')

OBJECTS_CPP = $(patsubst Source/%.cpp, $(OBJ)/BootManager/cpp/%.obj, $(SOURCES_CPP))
OBJECTS_ASM = $(patsubst Source/%.asm, $(OBJ)/BootManager/asm/%.obj, $(SOURCES_ASM))

build: dir $(OUTPUT)/BootMgr.exe

$(OUTPUT)/BootMgr.exe: $(OBJECTS_CPP) $(OBJECTS_ASM)
	$(MINGW_LDXX) $(MINGW_LINKFLAGS) -o $@ $^
	printf "\033[31mCreated -> $@\033[0m\n"

$(OBJ)/BootManager/cpp/%.obj: Source/%.cpp
	mkdir -p $(@D)
	$(MINGW_CXX) $(MINGW_CXXFLAGS) -c -o $@ $<
	printf "\033[31mCompiled -> $<\033[0m\n"

$(OBJ)/BootManager/asm/%.obj: Source/%.asm
	mkdir -p $(@D)
	$(X64_TARGET_ASM) $(X64_TARGET_ASMFLAGS) -o $@ $<
	printf "\033[31mAssembled -> $<\033[0m\n"

dir:
	mkdir -p $(OBJ)/BootManager/cpp
	mkdir -p $(OBJ)/BootManager/asm
