include scripts/config.mk

.PHONY: all tskschl tools tools-fat tools-image toolchain image dir clean

all: dir tskschl tools image

include scripts/toolchain.mk

tskschl:
	$(MAKE) -C $(SRC)/tskschl

tools: tools-fat tools-image

tools-image:
	$(MAKE) -C tools/Image

tools-fat:
	$(MAKE) -C tools/fat

image:
	$(OUTPUT)/image image.json $(OUTPUT)/OxizeOS.hdd

dir:
	mkdir -p $(OUTPUT)
	mkdir -p $(OBJ)
	mkdir -p $(OBJ)/tools/image
	mkdir -p $(OBJ)/tools/fat

clean:
	rm -rf $(OUTPUT)
	rm -rf $(OBJ)