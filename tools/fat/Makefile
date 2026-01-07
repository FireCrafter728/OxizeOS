CXXFLAGS += -O0 -fno-exceptions -fno-rtti -fPIC -I. -fshort-wchar -include global.hpp -g
LINKFLAGS += -pie

SOURCES_CPP=$(shell find . -name '*.cpp')
OBJECS_CPP=$(patsubst %.cpp, $(OBJ)/tools/fat/%.obj, $(SOURCES_CPP))

.PHONY: fat_build

fat_build: $(OUTPUT)/fat
$(OUTPUT)/fat: $(OBJECS_CPP)
	$(LDXX) $(LINKFLAGS) -o $@ $^
	echo "\033[92mCreated -> tools/fat\033[0m"

$(OBJ)/tools/fat/%.obj: %.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	echo "\033[36mCompiled -> $<\033[0m"