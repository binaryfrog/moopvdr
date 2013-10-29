#ifndef WINDOW_H
#define WINDOW_H

#include <glib.h>

#include "event.h"

typedef enum _WindowType WindowType;
typedef struct _Window Window;
typedef void (*WindowExposeFunc)(Window *window);
typedef gboolean (*WindowEventFunc)(Window *window, Event event);

enum _WindowType
{
	WINDOW_TOPLEVEL,
	WINDOW_DIALOG
};

struct _Window
{
	WindowType type;
	gboolean visible;
	gboolean dirty;
	WindowEventFunc event_func;
	WindowExposeFunc expose_func;
	gpointer user_data;
};

void window_manager_init();
gboolean window_manager_event(Event event);
void window_manager_run();

Window *window_new(WindowType type);
void window_delete(Window *window);

void window_hide(Window *window);
void window_show(Window *window);

void window_raise(Window *window);
void window_lower(Window *window);

#endif // not WINDOW_H
