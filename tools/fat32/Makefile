# SPDX-License-Identifier: GPL-3.0-or-later
# OxizeOS build system

CXXFLAGS += -fno-exceptions -fno-rtti -fPIC -I Include/ -O2
LINKFLAGS += -pie

SOURCES_CPP=$(shell find Source -name '*.cpp')
OBJECS_CPP=$(patsubst Source/%.cpp, $(OBJ)/tools/fat32/%.obj, $(SOURCES_CPP))

.PHONY: build

build: $(OUTPUT)/fat32
$(OUTPUT)/fat32: $(OBJECS_CPP)
	$(LDXX) $(LINKFLAGS) -o $@ $^
	printf "\033[1m\033[33m[FAT32]      \033[92mCreated -> $@\033[0m\n"

$(OBJ)/tools/fat32/%.obj: Source/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	printf "\033[1m\033[33m[FAT32]      \033[96mCompiled -> $<\033[0m\n"