#include <GL/gl.h>

#include "root_menu.h"
#include "window.h"
#include "font.h"

#include "schedule.h"
#include "recordings.h"
#include "conf.h"
#include "util.h"

typedef struct _RootMenuItem RootMenuItem;

struct _RootMenuItem
{
	char *name;
	Window **window;
};

static Window *RootMenuWindow;
static GList *RootMenuItems;
static RootMenuItem *RootMenuItemCurrent;

static gboolean root_menu_event(Window *window, Event event);
static void root_menu_expose(Window *window);
static void on_root_menu_up();
static void on_root_menu_down();
static void on_root_menu_select();

void root_menu_init()
{
	Window *window = window_new(WINDOW_TOPLEVEL);
	window->event_func = root_menu_event;
	window->expose_func = root_menu_expose;
	window_raise(window);
	RootMenuWindow = window;

	RootMenuItem *item;

	item = g_malloc(sizeof(RootMenuItem));
	item->name = "Timers";
	item->window = NULL;
	RootMenuItems = g_list_append(RootMenuItems, item);

	schedule_init();
	item = g_malloc(sizeof(RootMenuItem));
	item->name = "Schedule";
	item->window = &ScheduleWindow;
	RootMenuItems = g_list_append(RootMenuItems, item);

	recordings_init();
	item = g_malloc(sizeof(RootMenuItem));
	item->name = "Recordings";
	item->window = &RecordingsWindow;
	RootMenuItems = g_list_append(RootMenuItems, item);

	RootMenuItemCurrent = RootMenuItems->data;
}

static gboolean root_menu_event(Window *window, Event event)
{
	if(window != RootMenuWindow)
		return FALSE;

	switch(event)
	{
		case EVENT_UP:
			on_root_menu_up();
			return TRUE;
		case EVENT_DOWN:
			on_root_menu_down();
			return TRUE;
		case EVENT_SELECT:
			on_root_menu_select();
			return TRUE;
		default:
			return FALSE;
	}

	return FALSE;
}

static void root_menu_expose(Window *window)
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	viewport[0] += ConfScreenMarginX/2;
	viewport[1] += ConfScreenMarginY/2;
	viewport[2] -= ConfScreenMarginX/2;
	viewport[3] -= ConfScreenMarginY/2;

	font_set_element("heading");
	set_colour("normal", FALSE);

	int items = g_list_length(RootMenuItems);

	int height = items*font_get_line_height()/64;

	int x;
	int y = (viewport[1] + viewport[3] + height)/2;

	for(GList *i = RootMenuItems; i; i = i->next)
	{
		RootMenuItem *item = i->data;

		x = (viewport[0] + viewport[2])/2;

		if(item == RootMenuItemCurrent)
		{
			glPushAttrib(GL_CURRENT_BIT);
			set_colour("highlight", TRUE);
			glBegin(GL_QUADS);
			glVertex2i(0, y - ConfCellPaddingY);
			glVertex2i(ConfScreenWidth, y - ConfCellPaddingY);
			glVertex2i(ConfScreenWidth, y + font_get_line_height()/64 - ConfCellPaddingY);
			glVertex2i(0, y + font_get_line_height()/64 - ConfCellPaddingY);
			glEnd();
			glPopAttrib();
		}

		x -= (font_get_width(item->name)/64)/2;

		font_draw(item->name, x, y);

		y -= font_get_line_height()/64;
	}
}

static void on_root_menu_up()
{
	for(GList *i = RootMenuItems; i; i = i->next)
	{
		if(i->data == RootMenuItemCurrent)
		{
			if(i->prev)
			{
				RootMenuItemCurrent = i->prev->data;
				RootMenuWindow->dirty = TRUE;
			}
			return;
		}
	}
}

static void on_root_menu_down()
{
	for(GList *i = RootMenuItems; i; i = i->next)
	{
		if(i->data == RootMenuItemCurrent)
		{
			if(i->next)
			{
				RootMenuItemCurrent = i->next->data;
				RootMenuWindow->dirty = TRUE;
			}
			return;
		}
	}
}

static void on_root_menu_select()
{
	if(RootMenuItemCurrent->window)
	{
		(*RootMenuItemCurrent->window)->dirty = TRUE;
		window_show(*RootMenuItemCurrent->window);
		window_raise(*RootMenuItemCurrent->window);
	}
}

