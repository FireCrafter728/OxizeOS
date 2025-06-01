TARGET_ASMFLAGS += -f elf
TARGET_CFLAGS += -ffreestanding -nostdlib -I Drivers/HAL -I. -I util -I $(src)/libs -I Drivers
TARGET_LIBS += -lgcc
TARGET_LINKFLAGS += -T linker.ld -nostdlib

HEADERS_C=$(shell find . -name '*.h')
SOURCES_C=$(shell find . -name '*.c')
HEADERS_ASM=$(shell find . -name '*.inc')
SOURCES_ASM=$(shell find . -name '*.asm')
OBJECTS_C=$(patsubst %.c, $(output)/kernel/c/%.obj, $(SOURCES_C))
OBJECTS_ASM=$(patsubst %.asm, $(output)/kernel/asm/%.obj, $(SOURCES_ASM))

.PHONY: all kernel clean always generate

all: kernel

kernel: generate $(output)/kernel.bin

$(output)/kernel.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	@$(TARGET_LD) $(TARGET_LINKFLAGS) -Wl,-Map=$(output)/kernel.map -o $@ $^ $(TARGET_LIBS)
	@echo "--> Created:  kernel.bin"

$(output)/kernel/c/%.obj: %.c $(HEADERS_C)
	@mkdir -p $(@D)
	@$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<
	@echo "--> Compiled: " $<

$(output)/kernel/asm/%.obj: %.asm $(HEADERS_ASM)
	@mkdir -p $(@D)
	@$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<
	@echo "--> Compiled: " $<

generate:
	$(scripts)/gen_isr.sh Drivers/HAL/gen_isr.c Drivers/HAL/gen_isr.inc