#include <sys/wait.h>
#include <SDL/SDL.h>
#include <glib.h>

#include "event.h"

pid_t EventChildPid;
int EventChildStatus;

static gboolean EventChildExited = FALSE;

static void event_on_sigchld(int signal);
static Event event_poll_sdl();
static Event on_sdl_key_down(SDL_Event *sdl_event);
static Event on_sdl_video_expose(SDL_Event *sdl_event);

void event_init()
{
	signal(SIGCHLD, event_on_sigchld);
}

Event event_poll()
{
	Event event;

	if((event = event_poll_sdl()) != EVENT_NONE)
		return event;

	if(EventChildExited)
	{
		EventChildExited = FALSE;
		return EVENT_CHILD_EXITED;
	}

	return EVENT_NONE;
}

static void event_on_sigchld(int signal)
{
	if((EventChildPid = waitpid(-1, &EventChildStatus, WNOHANG)) > 0)
		EventChildExited = TRUE;
}

static Event event_poll_sdl()
{
	SDL_Event sdl_event;

	if(SDL_PollEvent(&sdl_event))
	{
		switch(sdl_event.type)
		{
			case SDL_KEYDOWN:
				return on_sdl_key_down(&sdl_event);
			case SDL_VIDEOEXPOSE:
				return on_sdl_video_expose(&sdl_event);
		}
	}

	return EVENT_NONE;
}

static Event on_sdl_key_down(SDL_Event *sdl_event)
{
	switch(sdl_event->key.keysym.sym)
	{
		case SDLK_q:		return EVENT_EXIT;
		case SDLK_RETURN:	return EVENT_SELECT;
		case SDLK_ESCAPE:	return EVENT_BACK;
		case SDLK_UP:		return EVENT_UP;
		case SDLK_DOWN:		return EVENT_DOWN;
		case SDLK_LEFT:		return EVENT_LEFT;
		case SDLK_RIGHT:	return EVENT_RIGHT;
		case SDLK_i:		return EVENT_INFO;
		case SDLK_r:		return EVENT_RED;
		case SDLK_g:		return EVENT_GREEN;
		case SDLK_y:		return EVENT_YELLOW;
		case SDLK_b:		return EVENT_BLUE;
		case SDLK_PAGEUP:	return EVENT_CHANNEL_UP;
		case SDLK_PAGEDOWN:	return EVENT_CHANNEL_DOWN;
		default:		return EVENT_NONE;
	}
}

static Event on_sdl_video_expose(SDL_Event *sdl_event)
{
	return EVENT_EXPOSE;
}

