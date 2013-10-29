CC = gcc
CFLAGS = -Wall -O0 -g $(GTK_CFLAGS)
LIBS = $(GTK_LIBS)

GTK_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-2.0)

all: moopvdr

moopvdr_SOURCES = main.c separator.c recordings.c misc.c mplayer.c
moopvdr_OBJECTS = $(moopvdr_SOURCES:.c=.o)
moopvdr_DEPENDS = $(moopvdr_SOURCES:.c=.d)

include $(moopvdr_DEPENDS)

moopvdr: $(moopvdr_OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

.PHONY: clean
clean:
	-rm -f moopvdr
	-rm -f $(moopvdr_OBJECTS)
	-rm -f $(moopvdr_DEPENDS)

%.d: %.c
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :] *,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

