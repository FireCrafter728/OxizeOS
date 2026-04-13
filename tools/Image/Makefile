CXXFLAGS += -I Include -include Global.hpp

SOURCES_CPP=$(shell find Source -name '*.cpp')
OBJECTS_CPP=$(patsubst Source/%.cpp, $(OBJ)/tools/image/%.obj, $(SOURCES_CPP))

build: $(OUTPUT)/image

$(OUTPUT)/image: $(OBJECTS_CPP)
	$(LDXX) $(LINKFLAGS) -o $@ $^
	printf "\033[1m\033[33m[IMAGE]      \033[92mCreated -> $@\033[0m\n"

$(OBJ)/tools/image/%.obj: Source/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	printf "\033[1m\033[33m[IMAGE]      \033[96mCompiled -> $<\033[0m\n"