CXXFLAGS += -O2 -fno-exceptions -fPIC -I. -fshort-wchar -include global.hpp
LINKFLAGS += -pie

SOURCES_CPP=$(shell find . -name '*.cpp')
OBJECS_CPP=$(patsubst %.cpp, $(OBJ)/tools/gpt/%.obj, $(SOURCES_CPP))

.PHONY: gpt_build

gpt_build: $(OUTPUT)/gpt
$(OUTPUT)/gpt: $(OBJECS_CPP)
	$(LDXX) $(LINKFLAGS) -o $@ $^
	printf "\033[1m\033[33m[GPT]        \033[92mCreated -> $@\033[0m\n"

$(OBJ)/tools/gpt/%.obj: %.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	printf "\033[1m\033[33m[GPT]        \033[96mCompiled -> $<\033[0m\n"