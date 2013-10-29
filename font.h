#ifndef FONT_H
#define FONT_H

#include <glib.h>

void font_init();

void font_set_face(const char *name);
void font_set_size(float pixel_size);

void font_set_element(const char *element);

int font_get_width(const char *s);
int font_get_line_height();
int font_get_height();
int font_get_ascender();
int font_get_descender();

int font_get_wrapped_lines(const char *s, int width);

void font_draw(const char *s, int x, int y);
void font_draw_wrapped(const char *s, int x, int y, int width);

#endif // not FONT_H
