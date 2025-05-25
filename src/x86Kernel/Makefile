build: dir $(output)/x86Kern.exe

TARGET_CFLAGS += -c -Wall -Wextra -nostdlib -ffreestanding -fPIE -I $(src)/libs/ -I . -I Standart/
TARGET_LINKFLAGS += -nostdlib -pie
TARGET_ASMFLAGS += -f elf32
TARGET_LIBS += -lgcc

SOURCES_C=$(shell find . -name '*.c')
SOURCES_ASM=$(shell find . -name '*.asm')

OBJECTS_C=$(patsubst %.c, $(output)/x86Kernel/c/%.obj, $(SOURCES_C))
OBJECTS_ASM=$(patsubst %.asm, $(output)/x86Kernel/asm/%.obj, $(SOURCES_ASM))

$(output)/x86Kern.exe: $(OBJECTS_C) $(OBJECTS_ASM)
	$(TARGET_LD) $(TARGET_LINKFLAGS) -o $@ $^ $(TARGET_LIBS)
	echo "Created -> $@"

$(output)/x86Kernel/c/%.obj: %.c
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -o $@ $<
	echo "Compiled -> $<"

$(output)/x86Kernel/asm/%.obj: %.asm
	@mkdir -p $(@D)
	$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<
	echo "Assembled -> $<"

dir:
	mkdir -p $(output)/x86Kernel
	mkdir -p $(output)/x86Kernel/c
	mkdir -p $(output)/x86Kernel/asm