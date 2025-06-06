TARGET_CFLAGS += -nostdlib -ffreestanding -I Include -nostdinc
TARGET_CXXFLAGS += -nostdlib -ffreestanding -I Include -nostdinc
TARGET_ARFLAGS += rcs

SOURCES_C=$(shell find src -name '*.c')
SOURCES_CPP=$(shell find src -name '*.cpp')

OBJECTS_C=$(patsubst src/%.c, $(output)/libstdcpp/obj/c/%.obj, $(SOURCES_C))
OBJECTS_CPP=$(patsubst src/%.cpp, $(output)/libstdcpp/obj/cpp/%.obj, $(SOURCES_CPP))

build: dir $(lib)/libstdcpp.lib copy
$(lib)/libstdcpp.lib: $(OBJECTS_C) $(OBJECTS_CPP)
	@mkdir -p $(@D)
	$(TARGET_AR) $(TARGET_ARFLAGS) -o $@ $^
	echo "Created -> $@"

$(output)/libstdcpp/obj/c/%.obj: src/%.c
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<
	echo "Compiled -> $<"

$(output)/libstdcpp/obj/cpp/%.obj: src/%.cpp
	@mkdir -p $(@D)
	$(TARGET_CXX) $(TARGET_CXXFLAGS) -c -o $@ $<
	echo "Compiled -> $<"

dir:
	mkdir -p $(output)/libstdcpp
	mkdir -p $(output)/libstdcpp/obj
	mkdir -p $(output)/libstdcpp/obj/c
	mkdir -p $(output)/libstdcpp/obj/cpp
	mkdir -p $(lib)/Include

copy:
	cp Include/* $(lib)/Include