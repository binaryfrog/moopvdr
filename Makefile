CC = gcc
CFLAGS = -std=c99 -Wall -O0 -g $(GLIB_CFLAGS) $(SDL_CFLAGS) $(FREETYPE_CFLAGS)
LIBS = $(GLIB_LIBS) $(SDL_LIBS) $(FREETYPE_LIBS) -lGL -lGLU

GLIB_CFLAGS := $(shell pkg-config --cflags glib-2.0)
GLIB_LIBS := $(shell pkg-config --libs glib-2.0)

SDL_CFLAGS := $(shell sdl-config --cflags)
SDL_LIBS := $(shell sdl-config --libs)

FREETYPE_CFLAGS := $(shell pkg-config --cflags freetype2)
FREETYPE_LIBS := $(shell pkg-config --libs freetype2)

targets = moopvdr
moopvdr_SOURCES = main.c conf.c display.c event.c font.c window.c root_menu.c vdr.c recordings.c util.c schedule.c
moopvdr_OBJECTS = $(moopvdr_SOURCES:.c=.o)
moopvdr_DEPENDS = $(moopvdr_SOURCES:.c=.d)

all: $(targets)

moopvdr: $(moopvdr_OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

-include $(moopvdr_DEPENDS)

.PHONY: clean
clean:
	-rm $(targets)
	-rm $(moopvdr_OBJECTS) $(moopvdr_DEPENDS)

%.d: %.c
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :] *,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

