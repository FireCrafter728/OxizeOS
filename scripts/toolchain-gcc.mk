TOOLCHAIN_PREFIX_X64 = $(TOOLCHAIN)/$(X64_TARGET)
export PATH := $(TOOLCHAIN_PREFIX_X64)/bin:$(PATH)

BINUTILS_SRC = $(TOOLCHAIN)/binutils-$(BINUTILS_VERSION)
BINUTILS_BUILD_X64 = $(TOOLCHAIN)/binutils-build-$(BINUTILS_VERSION)-X64

toolchain: toolchain_binutils_x64 toolchain_gcc_x64

toolchain_binutils_x64: $(TOOLCHAIN_PREFIX_X64)/bin/$(X64_TARGET)-ld

$(TOOLCHAIN_PREFIX_X64)/bin/$(X64_TARGET)-ld: $(BINUTILS_SRC).tar.gz
	cd $(BINUTILS_BUILD_X64) && CFLAGS= ASMFLAGS= CC= CXX= LD= LDXX= ASM= LINKFLAGS= LIBS= ../binutils-$(BINUTILS_VERSION)/configure \
		--prefix="$(TOOLCHAIN_PREFIX_X64)"	\
		--target=$(X64_TARGET)				\
		--with-sysroot						\
		--disable-nls						\
		--disable-werror
	$(MAKE) -j8 -C $(BINUTILS_BUILD_X64)
	$(MAKE) -C $(BINUTILS_BUILD_X64) install

$(BINUTILS_SRC).tar.gz:
	mkdir -p $(TOOLCHAIN) 
	cd $(TOOLCHAIN) && wget $(BINUTILS_URL)
	cd $(TOOLCHAIN) && tar -xf binutils-$(BINUTILS_VERSION).tar.gz
	mkdir $(BINUTILS_BUILD_X64)

GCC_SRC = $(TOOLCHAIN)/gcc-$(GCC_VERSION)
GCC_BUILD_X64 = $(TOOLCHAIN)/gcc-build-$(GCC_VERSION)-X64

toolchain_gcc_x64: $(TOOLCHAIN_PREFIX_X64)/bin/$(X64_TARGET)-gcc

$(TOOLCHAIN_PREFIX_X64)/bin/$(X64_TARGET)-gcc: $(TOOLCHAIN_PREFIX_X64)/bin/$(X64_TARGET)-ld $(GCC_SRC).tar.gz
	cd $(GCC_BUILD_X64) && CFLAGS= ASMFLAGS= CC= CXX= CXXFLAGS= LD= ASM= LINKFLAGS= LIBS= ../gcc-$(GCC_VERSION)/configure \
		--prefix="$(TOOLCHAIN_PREFIX_X64)" 			\
		--target=$(X64_TARGET)						\
		--disable-nls								\
		--enable-languages=c,c++					\
		--without-headers							\
		--with-pie									\
		--enable-default-pie						\
		--enable-initfini-array						\
		--build=$(shell $(TOOLCHAIN)/gcc-$(GCC_VERSION)/config.guess) \
		--host=$(shell $(TOOLCHAIN)/gcc-$(GCC_VERSION)/config.guess) \
		
	$(MAKE) -j8 -C $(GCC_BUILD_X64) all-gcc all-target-libgcc
	$(MAKE) -C $(GCC_BUILD_X64) install-gcc install-target-libgcc

$(GCC_SRC).tar.gz:
	mkdir -p $(TOOLCHAIN)
	cd $(TOOLCHAIN) && wget $(GCC_URL)
	cd $(TOOLCHAIN) && tar -xf gcc-$(GCC_VERSION).tar.gz
	mkdir $(GCC_BUILD_X64)

clean-toolchain:
	rm -rf $(GCC_BUILD_X64) $(GCC_SRC) $(BINUTILS_BUILD_X64) $(BINUTILS_SRC)

clean-toolchain-all:
	rm -rf $(TOOLCHAIN)

.PHONY: toolchain_binutils_x64 toolchain_gcc_x64 clean-toolchain clean-toolchain-all