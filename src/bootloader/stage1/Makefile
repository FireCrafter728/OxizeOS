.PHONY: all clean

all: stage1

stage1: $(output)/stage1.bin

$(output)/stage1.bin: boot.asm
	@$(ASM) $< -f bin -o $@
	@echo "--> Created  stage1.bin"