include scripts/config.mk

.PHONY: all libs disk_image mount x86Kern kernel bootloader clean clean-lib tools_fat tools_clean

all: dir disk_image

include scripts/toolchain.mk

disk_image: $(output)/OxizeOS.hdd

$(output)/OxizeOS.hdd: image bootloader kernel x86Kern tools_fat
	rm -f $@
	output/image image.json $@
#	scripts/make_disk_image.sh $@ $(MAKE_DISK_SIZE)

mount:
	@mkdir -p $(abspath ./OxizeOS_HDD)
	@scripts/MountDiskImage.sh $(output)/OxizeOS.hdd $(abspath ./OxizeOS_HDD)

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
tools_fat:
	@mkdir -p $(output)/tools
	@$(MAKE) -C tools/FAT32

tools_clean:
	@rm -rf $(output)/tools/*

#
# libs
#
libs:
	@$(MAKE) -C libstdcpp

#
# image
#
image:
	@$(MAKE) -C tools/Image

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