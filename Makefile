include scripts/config.mk

.PHONY: all disk_image x86Kern kernel bootloader clean tools_fat

all: dir disk_image tools_fat

include scripts/toolchain.mk

disk_image: $(output)/OxizeOS.hdd

$(output)/OxizeOS.hdd: bootloader kernel x86Kern
	@scripts/make_disk_image.sh $@ $(MAKE_DISK_SIZE)
	@echo "--> Created: " $@

#
# Bootloader
#
bootloader: stage1 stage2

stage1: $(output)/stage1.bin

$(output)/stage1.bin: dir
	@$(MAKE) -C src/bootloader/stage1

stage2: $(output)/stage2.bin

$(output)/stage2.bin: dir
	@$(MAKE) -C src/bootloader/stage2

#
# Kernel
#
kernel: $(output)/kernel.bin

$(output)/kernel.bin: dir
	@$(MAKE) -C src/kernel


x86Kern: $(output)/x86Kern.exe

$(output)/x86Kern.exe: 
	@$(MAKE) -C src/x86Kernel

#
# Tools
#
tools_fat: $(output)/tools/fat
$(output)/tools/fat: tools/fat/fat.c
	@mkdir -p $(output)/tools
	@$(MAKE) -C tools/fat

#
# dir
#
dir:
	@mkdir -p $(output)

#
# Clean
#
clean:
	@rm -rf $(output)
