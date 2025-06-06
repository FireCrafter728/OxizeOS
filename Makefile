include scripts/config.mk

.PHONY: all libs disk_image x86Kern kernel bootloader clean clean-lib tools_fat

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

#
# x86Kernel
#
x86Kern:
	@$(MAKE) -C src/x86Kernel

#
# Tools
#
tools_fat: $(output)/tools/fat
$(output)/tools/fat: tools/fat/fat.c
	@mkdir -p $(output)/tools
	@$(MAKE) -C tools/fat

#
# libs
#
libs:
	@$(MAKE) -C libstdcpp

#
# dir
#
dir:
	@mkdir -p $(output)
	@mkdir -p $(lib)

#
# Clean
#
clean:
	@rm -rf $(output)

clean-lib:
	@rm -rf $(lib)