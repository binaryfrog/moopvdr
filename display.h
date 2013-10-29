#ifndef DISPLAY_H
#define DISPLAY_H

#include <SDL/SDL_video.h>

extern SDL_Surface *Surface;

void display_init(int width, int height);
void display_destroy();

#endif // not DISPLAY_H
