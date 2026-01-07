TOOLCHAIN_PREFIX = $(TOOLCHAIN)/$(TARGET)
export PATH := $(TOOLCHAIN_PREFIX)/bin:$(PATH)

toolchain: toolchain_binutils toolchain_gcc

BINUTILS_SRC = $(TOOLCHAIN)/binutils-$(BINUTILS_VERSION)
BINUTILS_BUILD = $(TOOLCHAIN)/binutils-build-$(BINUTILS_VERSION)

toolchain_binutils: $(TOOLCHAIN_PREFIX)/bin/$(TARGET)-ld

$(TOOLCHAIN_PREFIX)/bin/$(TARGET)-ld: $(BINUTILS_SRC).tar.gz
	cd $(TOOLCHAIN) && tar -xf binutils-$(BINUTILS_VERSION).tar.gz
	mkdir $(BINUTILS_BUILD)
	cd $(BINUTILS_BUILD) && CFLAGS= ASMFLAGS= CC= CXX= LD= LDXX= ASM= LINKFLAGS= LIBS= ../binutils-$(BINUTILS_VERSION)/configure \
		--prefix="$(TOOLCHAIN_PREFIX)"	\
		--target=$(TARGET)				\
		--with-sysroot					\
		--disable-nls					\
		--disable-werror
	$(MAKE) -j8 -C $(BINUTILS_BUILD)
	$(MAKE) -C $(BINUTILS_BUILD) install

$(BINUTILS_SRC).tar.gz:
	mkdir -p $(TOOLCHAIN) 
	cd $(TOOLCHAIN) && wget $(BINUTILS_URL)


GCC_SRC = $(TOOLCHAIN)/gcc-$(GCC_VERSION)
GCC_BUILD = $(TOOLCHAIN)/gcc-build-$(GCC_VERSION)

toolchain_gcc: $(TOOLCHAIN_PREFIX)/bin/$(TARGET)-gcc

$(TOOLCHAIN_PREFIX)/bin/$(TARGET)-gcc: $(TOOLCHAIN_PREFIX)/bin/$(TARGET)-ld $(GCC_SRC).tar.gz
	mkdir $(GCC_BUILD)
	cd $(GCC_BUILD) && CFLAGS= ASMFLAGS= CC= CXX= CXXFLAGS= LD= ASM= LINKFLAGS= LIBS= ../gcc-$(GCC_VERSION)/configure \
		--prefix="$(TOOLCHAIN_PREFIX)" 	\
		--target=$(TARGET)				\
		--disable-nls					\
		--enable-languages=c,c++		\
		--without-headers				\
		--build=$(shell $(TOOLCHAIN)/gcc-$(GCC_VERSION)/config.guess) \
		--host=$(shell $(TOOLCHAIN)/gcc-$(GCC_VERSION)/config.guess) \
		
	$(MAKE) -j8 -C $(GCC_BUILD) all-gcc all-target-libgcc
	$(MAKE) -C $(GCC_BUILD) install-gcc install-target-libgcc
	
$(GCC_SRC).tar.gz:
	mkdir -p $(TOOLCHAIN)
	cd $(TOOLCHAIN) && wget $(GCC_URL)
	cd $(TOOLCHAIN) && tar -xf gcc-$(GCC_VERSION).tar.gz

clean-toolchain:
	rm -rf $(GCC_BUILD) $(GCC_SRC) $(BINUTILS_BUILD) $(BINUTILS_SRC)

clean-toolchain-all:
	rm -rf $(TOOLCHAIN)

.PHONY: toolchain toolchain_binutils toolchain_gcc clean-toolchain clean-toolchain-all