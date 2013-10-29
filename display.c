#include <SDL/SDL.h>
#include <GL/gl.h>
#include <glib.h>

#include "display.h"
#include "conf.h"

SDL_Surface *Surface;

void display_init(int width, int height)
{
	SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);

	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,
			SDL_DEFAULT_REPEAT_INTERVAL);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

	if(ConfScreenHideCursor)
		SDL_ShowCursor(FALSE);

	int flags = SDL_OPENGL;

	if(ConfScreenFullscreen)
		flags |= SDL_FULLSCREEN;

	Surface = SDL_SetVideoMode(width, height, 32, flags);

	SDL_WM_SetCaption("MoopVdr", NULL);

	g_debug("vendor: %s", glGetString(GL_VENDOR));
	g_debug("renderer: %s", glGetString(GL_RENDERER));
	g_debug("version: %s", glGetString(GL_VERSION));
	g_debug("extensions: %s", glGetString(GL_EXTENSIONS));

	GLint max_texture_size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
	g_debug("GL_MAX_TEXTURE_SIZE=%d", max_texture_size);
}

void display_destroy()
{
	if(Surface)
	{
		SDL_FreeSurface(Surface);
		SDL_Quit();
	}
}

