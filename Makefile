# SPDX-License-Identifier: GPL-3.0-or-later
# OxizeOS build system

include scripts/config.mk

.PHONY: all bootmgr syskrnl64 tools tools-fat tools-image tools-gpt toolchain image dir clean

all: dir bootmgr syskrnl64 tools image

include scripts/toolchain-gcc.mk

bootmgr:
	$(MAKE) -C $(SRC)/BootManager

syskrnl64:
	$(MAKE) -C $(SRC)/syskrnl64

tools: tools-fat tools-image tools-gpt

tools-image:
	$(MAKE) -C tools/Image

tools-fat:
	$(MAKE) -C tools/fat

tools-gpt:
	$(MAKE) -C tools/gpt

image: bootmgr syskrnl64 tools
	$(OUTPUT)/image image.conf

dir:
	mkdir -p $(OUTPUT)
	mkdir -p $(OBJ)
	mkdir -p $(OBJ)/tools/fat
	mkdir -p $(OBJ)/tools/image
	mkdir -p $(OBJ)/tools/gpt

clean:
	rm -rf $(OUTPUT)
	rm -rf $(OBJ)