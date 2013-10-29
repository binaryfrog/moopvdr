#include <GL/gl.h>

#include "channels.h"
#include "vdr.h"
#include "font.h"

#define HEADING		"Channels"

Window *ChannelsWindow;

static GList *VdrChannels;
static GList *Top;
static GList *Current;

static gboolean channels_event(Window *window, Event event);
static void channels_expose(Window *window);
static void on_channels_up();
static void on_channels_down();

void channels_init()
{
	Window *window;
	
	Channelswindow = window = window_new(WINDOW_TOPLEVEL);
	window->event_func = channels_event;
	window->expose_func = channels_expose;
	
	VdrChannels = vdr_get_channel_list();
	CurrentVdrChannel = VdrChannels->data;
}

static gboolean channels_event(Window *window, Event event)
{
	if(window != ChannelsWindow)
		return FALSE;

	switch(event)
	{
		case EVENT_UP:
			on_channels_up();
			return TRUE;
		case EVENT_DOWN:
			on_channels_down();
			return TRUE;
		case EVENT_BACK:
			g_debug("lowering channels window");
			window_lower(ChannelsWindow);
			return TRUE;
		default:
			break;
	}

	return FALSE;
}

static void channels_expose(Window *window)
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	viewport[0] += ConfScreenMarginX/2;
	viewport[1] += ConfScreenMarginY/2;
	viewport[2] -= ConfScreenMarginX/2;
	viewport[3] -= ConfScreenMarginY/2;

	set_colour("heading", FALSE);
	font_set_element("heading");
	int width = font_get_width(HEADING)/64;
}

static void on_channels_up()
{
	if(!Current->prev)
		return;
	
	if(Current == Top)
		Top = Top->prev;

	Current = Current->prev;

	Channelswindow->dirty = TRUE;
}

static void on_channels_down()
{
	if(!Current->next)
		return;

	Current = Current->next;

	int diff = g_list_position(VdrChannels, Current)
		- g_list_position(VdrChannels, Top);

	if(DisplayRows && diff > DisplayRows)
		Top = Top->next;

	ChannelsWindow->dirty = TRUE;
}

