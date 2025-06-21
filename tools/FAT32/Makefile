CXXFLAGS += -O2 -fno-exceptions -fno-rtti -fPIC -m32 -I. -fshort-wchar -include global.hpp
LINKFLAGS += -m32 -pie

SOURCES_CPP=$(shell find . -name '*.cpp')
OBJECS_CPP=$(patsubst %.cpp, $(output)/tools/fat32/%.obj, $(SOURCES_CPP))

.PHONY: fat32_build fat32_clean

fat32_build: $(output)/tools/fat
$(output)/tools/fat: $(OBJECS_CPP)
	$(LDXX) $(LINKFLAGS) -o $@ $^
	echo "Created -> tools/fat"

$(output)/tools/fat32/%.obj: %.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	echo "Compiled -> $<"