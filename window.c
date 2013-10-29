#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "window.h"
#include "common.h"
#include "conf.h"
#include "util.h"

static GQueue *WindowList;
static GList *DialogList;

static void window_manager_begin_draw();
static void window_manager_end_draw();

void window_manager_init()
{
	WindowList = g_queue_new();
	DialogList = NULL;

	for(GList *i = ConfColours; i; i = i->next)
	{
		ConfColour *cc = i->data;

		if(strcmp(cc->element, "background") == 0)
		{
			glClearColor(cc->rgba[0]/256.0,
					cc->rgba[1]/256.0,
					cc->rgba[2]/256.0,
					1.0);
			break;
		}
	}

	glShadeModel(GL_SMOOTH);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

gboolean window_manager_event(Event event)
{
	Window *receiver = WindowList->head->data;
	Window *window;

	for(GList *i = DialogList; i; i = i->next)
	{
		window = i->data;
		if(window->visible)
		{
			receiver = window;
			break;
		}
	}

	for(GList *i = DialogList; i; i = i->next)
	{
		window = i->data;
		if(window->visible && window->event_func(receiver, event))
			return TRUE;
	}

	for(GList *i = WindowList->head; i; i = i->next)
	{
		window = i->data;
		if(window->event_func(receiver, event))
			return TRUE;
	}

	if(event == EVENT_EXPOSE)
	{
		if(WindowList->head)
		{
			window = WindowList->head->data;
			window->dirty = TRUE;
		}

		for(GList *i = DialogList; i; i = i->next)
		{
			window = i->data;
			window->dirty = TRUE;
		}

		return TRUE;
	}

	return FALSE;
}

void window_manager_run()
{
	Window *window;
	gboolean dirty = FALSE;

	for(GList *i = DialogList; i; i = i->next)
	{
		window = i->data;

		if(window->visible && window->dirty)
			dirty = TRUE;
	}

	if(WindowList->head)
	{
		window = WindowList->head->data;

		if(dirty || window->dirty)
		{
			dirty = TRUE;
			window_manager_begin_draw();
			window->expose_func(window);
			window->dirty = FALSE;
		}
	}

	for(GList *i = DialogList; i; i = i->next)
	{
		window = i->data;

		if(window->visible && window->dirty)
		{
			if(!dirty)
			{
				dirty = TRUE;
				window_manager_begin_draw();
			}

			GLint viewport[4];
			glGetIntegerv(GL_VIEWPORT, viewport);

			glPushAttrib(GL_CURRENT_BIT);
			set_colour("dialog-shade", TRUE);
			glBegin(GL_QUADS);
			glVertex2i(viewport[0], viewport[1]);
			glVertex2i(viewport[0] + viewport[2], viewport[1]);
			glVertex2i(viewport[0] + viewport[2],
					viewport[1] + viewport[3]);
			glVertex2i(viewport[0], viewport[1] + viewport[3]);
			glEnd();
			glPopAttrib();

			window->expose_func(window);
			window->dirty = FALSE;

			break;
		}
	}

	if(dirty)
		window_manager_end_draw();
}

Window *window_new(WindowType type)
{
	Window *window = g_new0(Window, 1);
	window->type = type;
	window->visible = (type == WINDOW_TOPLEVEL);
	window->dirty = TRUE;

	if(window->type == WINDOW_TOPLEVEL)
		g_queue_push_tail(WindowList, window);
	else
		DialogList = g_list_append(DialogList, window);

	return window;
}

void window_delete(Window *window)
{
	g_assert(window != NULL);

	if(window->type == WINDOW_TOPLEVEL)
		g_queue_remove(WindowList, window);
	else
		DialogList = g_list_remove(DialogList, window);

	g_free(window);
}


void window_hide(Window *window)
{
	if(window->type == WINDOW_DIALOG)
	{
		window->visible = FALSE;
		window = WindowList->head->data;
		window->dirty = TRUE;
	}
}

void window_show(Window *window)
{
	if(window->type == WINDOW_DIALOG)
	{
		window->visible = TRUE;
		window->dirty = TRUE;
		g_debug("window_show()");
	}
}

void window_raise(Window *window)
{
	Window *head = WindowList->head->data;
	head->event_func(head, EVENT_UNMAPPED);

	g_queue_remove(WindowList, window);
	g_queue_push_head(WindowList, window);
	window->event_func(window, EVENT_MAPPED);
	window->dirty = TRUE;
}

void window_lower(Window *window)
{
	g_queue_remove(WindowList, window);
	g_queue_push_tail(WindowList, window);
	window->event_func(window, EVENT_UNMAPPED);

	Window *head = WindowList->head->data;
	window->event_func(head, EVENT_MAPPED);
	head->dirty = TRUE;
}

static void window_manager_begin_draw()
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(viewport[0], viewport[2], viewport[1], viewport[3]);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void window_manager_end_draw()
{
	SDL_GL_SwapBuffers();
}

