#include <gtk/gtk.h>

#include "separator.h"

static void on_size_request(GtkWidget *, GtkRequisition *);
static void on_size_allocate(GtkWidget *, GtkAllocation *);
static gboolean on_expose_event(GtkWidget *, GdkEventExpose *);

GtkWidget *separator_new()
{
	GtkWidget *draw_area;

	draw_area = gtk_drawing_area_new();
	g_signal_connect(G_OBJECT(draw_area), "size-request",
			G_CALLBACK(on_size_request), NULL);
	g_signal_connect(G_OBJECT(draw_area), "size-allocate",
			G_CALLBACK(on_size_allocate), NULL);
	g_signal_connect(G_OBJECT(draw_area), "expose-event",
			G_CALLBACK(on_expose_event), NULL);

	return draw_area;
}

static void on_size_request(GtkWidget *w, GtkRequisition *r)
{
	r->width = 100;
	r->height = 4;
}

static void on_size_allocate(GtkWidget *w, GtkAllocation *a)
{
	g_debug("separator allocated %dx%d (at %d,%d)",
			a->width, a->height, a->x, a->y);
}

static gboolean on_expose_event(GtkWidget *w, GdkEventExpose *e)
{
	int y;
	GtkStyle *style = gtk_rc_get_style(w);

	g_debug("separator exposed %dx%d (at %d,%d)",
			e->area.width, e->area.height,
			e->area.x, e->area.y);

	for(y = 0; y < e->area.height; y++)
	{
		gdk_draw_line(GDK_DRAWABLE(e->window),
				(e->area.y + y < 2
				 ? style->dark_gc[GTK_STATE_NORMAL]
				 : style->light_gc[GTK_STATE_NORMAL]),
				e->area.x, e->area.y + y,
				e->area.x + e->area.width, e->area.y + y);
	}

	return TRUE;
}

