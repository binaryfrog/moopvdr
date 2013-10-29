#include "dialog.h"
#include "conf.h"
#include "font.h"

static gboolean event_func(Window *window, Event event);
static void expose_func(Window *window);

Dialog *dialog_new(const char *header, const char *body)
{
	Dialog *dialog = g_new0(Dialog, 1);

	dialog->window = window_new(WINDOW_DIALOG);
	dialog->window->event_func = event_func;
	dialog->window->expose_func = expose_func;
	dialog->window->user_data = dialog;
	window_hide(dialog->window);

	if(header)
		dialog->header = g_strdup(header);

	if(body)
		dialog->body = g_strdup(body);

	dialog->response = DIALOG_RESPONSE_NONE;

	return dialog;
}

void dialog_delete(Dialog *dialog)
{
	g_assert(dialog != NULL);

	window_delete(dialog->window);

	if(dialog->header)
		g_free(dialog->header);

	if(dialog->body)
		g_free(dialog->body);

	for(GList *i = dialog->buttons; i; i = i->next)
	{
		DialogButton *button = i->data;
		g_free(button->label);
		g_free(button);
	}

	g_list_free(dialog->buttons);
}

static gboolean event_func(Window *window, Event event)
{
	Dialog *dialog = window->user_data;

	switch(event)
	{
		default:
			break;
	}

	return FALSE;
}

static void expose_func(Window *window)
{
	Dialog *dialog = window->user_data;

	int width = ConfDialogWidth;
	int header_height = 0;
	int buttons_height = 0;
	int body_height = 0;

	if(header)
	{
		font_set_element("dialog-heading");
		width = MAX(width, font_get_width(dialog->header)/64);
		header_height = font_get_height()/64 + 2*ConfDialogMargin;
	}

	if(buttons)
	{
		font_set_element("button");

		int button_width = 0;
		for(GList *i = dialog->buttons; i; i = i->next)
		{
			DialogButton *button = i->data;
			button_width += 2*ConfButtonPadding;
			button_width += font_get_width(button->label)/64;
			if(i->next)
				button_width += ConfButtonSpacing;
		}

		width = MAX(width, button_width);

		buttons_height = font_get_height()/64;
		buttons_height += 2*(ConfButtonPadding + ConfDialogMargin);
	}

	if(body)
	{
		font_set_element("dialog-body");
		int lines = font_get_wrapped_lines(dialog->body, width);
		body_height = lines*font_get_line_height()/64;
		body_height += 2*ConfDialogMargin;
	}

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	int x = viewport[0] + (viewport[2] - viewport[0])/2;
	int y = viewport[1] + (viewport[3] - viewport[1])/2;
	int height = height = header_height + body_height + buttons_height;

	x -= width/2;
	y -= height/2;

	if(header)
	{
		set_colour("dialog-heading", TRUE);
		glBegin(GL_QUADS);
		glVertex2i(x - ConfDialogMargin, y - ConfDialogMargin);
		glVertex2i(x + width + ConfDialogMargin, y - ConfDialogMargin);
		glVertex2i(x + width + ConfDialogMargin,
				y + header_height + ConfDialogMargin);
		glVertex2i(x - ConfDialogMargin,
				y + header_height + ConfDialogMargin);
		glEnd();

		font_set_element("dialog-heading");
		font_draw(dialog->header, x, y + font_get_height()/64);

		y += header_height();
	}

	if(body)
	{
		set_colour("dialog-body", TRUE);
		glBegin(GL_QUADS);
		glVertex2i(x - ConfDialogMargin, y - ConfDialogMargin);
		glVertex2i(x + width + ConfDialogMargin, y - ConfDialogMargin);
		glVertex2i(x + width + ConfDialogMargin,
				y + body_height + ConfDialogMargin);
		glVertex2i(x - ConfDialogMargin,
				y + body_height + ConfDialogMargin);
		glEnd();

		font_set_element("dialog-body", TRUE);
		font_draw_wrapped(dialog->body, x, y + font_get_height()/64);

		y += body_height;
	}

	if(buttons)
	{
		set_colour("dialog-button", TRUE);
		glBegin(GL_QUADS);
		glVertex2i(x - ConfDialogMargin, y - ConfDialogMargin);
		glVertex2i(x + width + ConfDialogMargin, y - ConfDialogMargin);
		glVertex2i(x + width + ConfDialogMargin,
				y + buttons_height + ConfDialogMargin);
		glVertex2i(x - ConfDialogMargin,
				y + buttons_height + ConfDialogMargin);
		glEnd();

		font_set_element("button", TRUE);

		int this_x = x;

		for(GList *i = dialog->buttons; i; i = i->next)
		{
			DialogButton *button = i->data;
			this_x += ConfButtonMargin;
			glBegin(GL_QUADS);
			glVertex2i(this_x, y + ConfDialogMargin
		}
	}
}

