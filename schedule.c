#include <SDL/SDL.h>
#include <GL/gl.h>

#include "schedule.h"
#include "vdr.h"
#include "font.h"
#include "conf.h"
#include "util.h"

Window *ScheduleWindow;

static GList *VdrChannels;
static GList *CurrentChannel;
static GList *VdrEvents;
static GList *CurrentEvent;
static GList *TopEvent;
static GList *BottomEvent;

static gboolean schedule_event(Window *window, Event event);
static void schedule_expose(Window *window);
static void on_schedule_up();
static void on_schedule_down();
static void on_schedule_channel_up();
static void on_schedule_channel_down();

void schedule_init()
{
	Window *window;

	ScheduleWindow = window = window_new(WINDOW_TOPLEVEL);
	window->event_func = schedule_event;
	window->expose_func = schedule_expose;

	VdrChannels = vdr_get_channel_list();
	CurrentChannel = VdrChannels;

	VdrChannel *channel = CurrentChannel->data;
	VdrEvents = vdr_get_events_list(channel->number);
	TopEvent = BottomEvent = CurrentEvent = VdrEvents;
}

static gboolean schedule_event(Window *window, Event event)
{
	if(window != ScheduleWindow)
		return FALSE;

	switch(event)
	{
		case EVENT_UP:
			on_schedule_up();
			return TRUE;
		case EVENT_DOWN:
			on_schedule_down();
			return TRUE;
		case EVENT_CHANNEL_UP:
			on_schedule_channel_up();
			return TRUE;
		case EVENT_CHANNEL_DOWN:
			on_schedule_channel_down();
			return TRUE;
		case EVENT_BACK:
			window_lower(ScheduleWindow);
			return TRUE;
		default:
			break;
	};

	return FALSE;
}

static void schedule_expose(Window *window)
{
	VdrChannel *channel = CurrentChannel->data;

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	viewport[0] += ConfScreenMarginX/2;
	viewport[1] += ConfScreenMarginY/2;
	viewport[2] -= ConfScreenMarginX/2;
	viewport[3] -= ConfScreenMarginY/2;

	font_set_element("heading");
	set_colour("heading", FALSE);
	int width = font_get_width(channel->name)/64;
	font_draw(channel->name,
			viewport[0] + (viewport[2] - viewport[0] - width)/2,
			viewport[3] - font_get_height()/64
			- ConfHeadingPaddingY);
	viewport[3] -= 2*ConfHeadingPaddingY + font_get_height()/64;

	font_set_element("normal");

	int col_widths[1] = { 0 };
	for(GList *i = VdrEvents; i; i = i->next)
	{
		VdrEvent *event = i->data;
		width = font_get_width(event->formatted_time)/64.0;
		col_widths[0] = MAX(col_widths[0], width);
	}

	col_widths[0] += 2*ConfCellPaddingX;

	int y = viewport[3];
	int x = 0;

	int row_height = font_get_height()/64 + 2*ConfCellPaddingY;
	y -= row_height;

	set_colour("normal", FALSE);

	GList *i;
	for(i = TopEvent; i; i = i->next)
	{
		if(y - row_height < viewport[1])
			break;

		VdrEvent *event = i->data;

		/*if(i->prev)
		{
			if(y - 2*row_height < viewport[1])
				break;

			struct tm *tm;

			tm = localtime(&event->start_time);
			int current_day = tm->tm_mday;

			VdrEvent *prev = i->prev->data;
			tm = localtime(&prev->start_time);
			int prev_day = tm->tm_mday;

			if(current_day != prev_day)
			{
				set_colour("column-heading-bg", TRUE);
				glBegin(GL_QUADS);
				glVertex2i(0, y - ConfCellPaddingY);
				glVertex2i(ConfScreenWidth, y - ConfCellPaddingY);
				glVertex2i(ConfScreenWidth, y + row_height - ConfCellPaddingY);
				glVertex2i(0, y + row_height - ConfCellPaddingY);
				glEnd();
				set_colour("normal", FALSE);
				y -= row_height;
			}
		}*/

		if(i == CurrentEvent)
		{
			set_colour("highlight", TRUE);
			glBegin(GL_QUADS);
			glVertex2i(0, y - ConfCellPaddingY);
			glVertex2i(ConfScreenWidth, y - ConfCellPaddingY);
			glVertex2i(ConfScreenWidth, y + row_height - ConfCellPaddingY);
			glVertex2i(0, y + row_height - ConfCellPaddingY);
			glEnd();
			set_colour("normal", FALSE);
		}

		width = font_get_width(event->formatted_time)/64;
		x = viewport[0] + col_widths[0] - width - ConfCellPaddingX;
		font_draw(event->formatted_time, x, y);
		x = viewport[0] + col_widths[0] + ConfCellPaddingX;
		font_draw(event->title, x, y);
		y -= row_height;

		if(y < viewport[1])
			break;
	}

	BottomEvent = (i ? i : g_list_last(VdrEvents));
}

static void on_schedule_up()
{
	if(!CurrentEvent->prev)
		return;

	if(CurrentEvent == TopEvent)
		TopEvent = TopEvent->prev;

	CurrentEvent = CurrentEvent->prev;

	ScheduleWindow->dirty = TRUE;
}

static void on_schedule_down()
{
	if(!CurrentEvent->next)
		return;

	CurrentEvent = CurrentEvent->next;

	if(CurrentEvent == BottomEvent)
		TopEvent = TopEvent->next;

	ScheduleWindow->dirty = TRUE;
}

static void on_schedule_channel_up()
{
	if(!CurrentChannel->prev)
		return;

	CurrentChannel = CurrentChannel->prev;

	VdrChannel *channel = CurrentChannel->data;
	vdr_free_events_list(VdrEvents);
	VdrEvents = vdr_get_events_list(channel->number);
	TopEvent = CurrentEvent = VdrEvents;
	BottomEvent = NULL;

	ScheduleWindow->dirty = TRUE;
}

static void on_schedule_channel_down()
{
	if(!CurrentChannel->next)
		return;

	CurrentChannel = CurrentChannel->next;

	VdrChannel *channel = CurrentChannel->data;
	vdr_free_events_list(VdrEvents);
	VdrEvents = vdr_get_events_list(channel->number);
	TopEvent = CurrentEvent = VdrEvents;
	BottomEvent = NULL;

	ScheduleWindow->dirty = TRUE;
}

