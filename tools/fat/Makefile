SOURCES_C=$(wildcard *.c)
OBJECTS_C=$(patsubst %.c, $(output)/tools/fat/%.o, $(SOURCES_C))

.PHONY: all fat clean always

all: fat

fat: $(output)/tools/fat.out

$(output)/tools/fat.out: $(OBJECTS_ASM) $(OBJECTS_C)
	@$(CC) $(CFLAGS) -o $@ $(OBJECTS_C)

$(output)/tools/fat/%.o: %.c always
	@$(CC) $(CFLAGS) -c -o $@ $<

always:
	@mkdir -p $(output)/tools/fat