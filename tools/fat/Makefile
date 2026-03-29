CXXFLAGS += -fno-exceptions -fno-rtti -fPIC -I. -fshort-wchar -include global.hpp
LINKFLAGS += -pie

SOURCES_CPP=$(shell find . -name '*.cpp')
OBJECS_CPP=$(patsubst %.cpp, $(OBJ)/tools/fat/%.obj, $(SOURCES_CPP))

.PHONY: fat_build

fat_build: $(OUTPUT)/fat
$(OUTPUT)/fat: $(OBJECS_CPP)
	$(LDXX) $(LINKFLAGS) -o $@ $^
	printf "\033[1m\033[33m[FAT32]      \033[92mCreated -> tools/fat\033[0m\n"

$(OBJ)/tools/fat/%.obj: %.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	printf "\033[1m\033[33m[FAT32]      \033[96mCompiled -> $<\033[0m\n"