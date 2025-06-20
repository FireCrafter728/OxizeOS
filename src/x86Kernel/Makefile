TARGET_CFLAGS += -nostdlib -ffreestanding -fPIC -I $(src)/libs/ -I . -I Standart/ -I Drivers/ -I $(lib)/Include
TARGET_CXXFLAGS += -nostdlib -ffreestanding -fPIC -I -fno-rtti -fno-exceptions -I $(src)/libs/ -I . -I Standart/ -I Drivers/ -I $(lib)/Include
TARGET_LINKFLAGS += -nostdlib -pie -T linker.ld -L $(lib)
TARGET_ASMFLAGS += -f elf32
TARGET_LIBS += -l:libstdcpp.lib -lgcc 

SOURCES_C=$(shell find . -name '*.c')
HEADERS_C=$(shell find . -name '*.h')
SOURCES_CPP=$(shell find . -name '*.cpp')
HEADERS_CPP=$(shell find . -name '*.hpp')
SOURCES_ASM=$(shell find . -name '*.asm')
HEADERS_ASM=$(shell find . -name '*.inc')

OBJECTS_C=$(patsubst %.c, $(output)/x86Kernel/c/%.obj, $(SOURCES_C))
OBJECTS_CPP=$(patsubst %.cpp, $(output)/x86Kernel/cpp/%.obj, $(SOURCES_CPP))
OBJECTS_ASM=$(patsubst %.asm, $(output)/x86Kernel/asm/%.obj, $(SOURCES_ASM))

.PHONY: all generate dir build

all: dir build

build: generate $(output)/x86Kern.exe

$(output)/x86Kern.exe: $(OBJECTS_ASM) $(OBJECTS_C) $(OBJECTS_CPP)
	@$(TARGET_LDXX) $(TARGET_LINKFLAGS) -Wl,-Map=$(output)/x86kernel.map -o $@ $^ $(TARGET_LIBS)
	@echo "Created -> $@"

$(output)/x86Kernel/c/%.obj: %.c $(HEADERS_C)
	@mkdir -p $(@D)
	@$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<
	@echo "Compiled -> $<"

$(output)/x86Kernel/cpp/%.obj: %.cpp $(HEADERS_CPP)
	@mkdir -p $(@D)
	@$(TARGET_CXX) $(TARGET_CXXFLAGS) -c -o $@ $<
	@echo "Compiled -> $<"

$(output)/x86Kernel/asm/%.obj: %.asm $(HEADERS_ASM)
	@mkdir -p $(@D)
	@$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<
	@echo "Assembled -> $<"

dir:
	mkdir -p $(output)/x86Kernel
	mkdir -p $(output)/x86Kernel/c
	mkdir -p $(output)/x86Kernel/cpp
	mkdir -p $(output)/x86Kernel/asm