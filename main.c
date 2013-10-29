#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <glib.h>

#include "common.h"
#include "conf.h"
#include "display.h"
#include "event.h"
#include "font.h"
#include "window.h"
#include "root_menu.h"
#include "vdr.h"

static void on_event(Event event)
{
	if(event == EVENT_EXIT)
		exit(0);
}

static void cleanup()
{
	display_destroy();
}

int main(int argc, char **argv)
{
	atexit(cleanup);

	conf_read("moopvdr.conf");

	display_init(ConfScreenWidth, ConfScreenHeight);
	event_init();
	font_init();
	window_manager_init();
	vdr_init(ConfVdrHostname, ConfVdrPort);
	root_menu_init();

	Event event;

	for(;;)
	{
		if((event = event_poll()) != EVENT_NONE)
			if(!window_manager_event(event))
				on_event(event);

		window_manager_run();

		g_usleep(1000);
	}
}

