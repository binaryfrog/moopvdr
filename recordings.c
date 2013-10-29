#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <GL/gl.h>

#include "common.h"
#include "recordings.h"
#include "vdr.h"
#include "font.h"
#include "conf.h"
#include "util.h"

#define HEADING		"Recordings"
#define COLUMN_DATE	"Date"
#define COLUMN_TIME	"Time"
#define COLUMN_TITLE	"Title"

Window *RecordingsWindow;

static GList *VdrRecordings;
static GList *Current;
static GList *Top;
static int DisplayRows;
static Window *InfoWindow;
static Window *ConfirmDeleteWindow;
static VdrRecordingInfo *Info;
static int ConfirmDeleteSelection;

static gboolean recordings_event(Window *window, Event event);
static void recordings_expose(Window *window);
static void on_recordings_up();
static void on_recordings_down();
static void on_recordings_select();
static gint recordings_cmp(VdrRecording *a, VdrRecording *b);
static gboolean info_event(Window *window, Event event);
static void info_expose(Window *window);
static gboolean confirm_delete_event(Window *window, Event event);
static void confirm_delete_expose(Window *window);

void recordings_init()
{
	Window *window;

	RecordingsWindow = window = window_new(WINDOW_TOPLEVEL);
	window->event_func = recordings_event;
	window->expose_func = recordings_expose;

	InfoWindow = window = window_new(WINDOW_DIALOG);
	window->event_func = info_event;
	window->expose_func = info_expose;
	window_hide(window);

	ConfirmDeleteWindow = window = window_new(WINDOW_DIALOG);
	window->event_func = confirm_delete_event;
	window->expose_func = confirm_delete_expose;
	window_hide(window);
}

static void recordings_update()
{
	int top_id = -1, current_id = -1;

	if(VdrRecordings)
	{
		if(Top)
		{
			VdrRecording *recording = Top->data;
			top_id = recording->id;
		}

		if(Current)
		{
			VdrRecording *recording = Current->data;
			current_id = recording->id;
		}

		vdr_free_recordings_list(VdrRecordings);
		VdrRecordings = Top = Current = NULL;
	}

	VdrRecordings = vdr_get_recordings_list();
	VdrRecordings = g_list_sort(VdrRecordings, recordings_cmp);

	if(0 && (top_id != -1 || current_id != -1))
	{
		Top = Current = NULL;

		for(GList *i = VdrRecordings; i; i = i->next)
		{
			VdrRecording *recording = i->data;

			if(top_id == recording->id)
				Top = i;

			if(current_id == recording->id)
				Current = i;
		}

		if(!Current)
		{
			if(Top)
				Current = Top;
			else
				Current = VdrRecordings;
		}

		if(!Top)
			Top = Current;
	} else
		Top = Current = VdrRecordings;
}

static gboolean recordings_event(Window *window, Event event)
{
	VdrRecording *recording;

	if(window != RecordingsWindow)
		return FALSE;

	switch(event)
	{
		case EVENT_CHILD_EXITED:
			recordings_update();
			RecordingsWindow->dirty = TRUE;
			return TRUE;
		case EVENT_UP:
			on_recordings_up();
			return TRUE;
		case EVENT_DOWN:
			on_recordings_down();
			return TRUE;
		case EVENT_SELECT:
			on_recordings_select();
			return TRUE;
		case EVENT_BACK:
			window_lower(RecordingsWindow);
			return TRUE;
		case EVENT_INFO:
			recording = Current->data;
			Info = vdr_get_recording_info(recording->id);
			window_show(InfoWindow);
			return TRUE;
		case EVENT_BLUE:
			ConfirmDeleteSelection = 1;
			window_show(ConfirmDeleteWindow);
			return TRUE;
		case EVENT_MAPPED:
			g_debug("recordings mapped");
			recordings_update();
			return TRUE;
		case EVENT_UNMAPPED:
			g_debug("recordings unmapped");
			vdr_free_recordings_list(VdrRecordings);
			VdrRecordings = Top = Current = NULL;
			return TRUE;
		default:
			break;
	}

	return FALSE;
}

static void recordings_expose(Window *window)
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
	font_draw(HEADING,
			viewport[0] + (viewport[2] - viewport[0] - width)/2,
			viewport[3] - font_get_height()/64
			- ConfHeadingPaddingY);
	viewport[3] -= 2*ConfHeadingPaddingY + font_get_height()/64;

	font_set_element("normal");

	int col_widths[2] = { 0, 0 };
	for(GList *i = VdrRecordings; i; i = i->next)
	{
		VdrRecording *recording = i->data;
		width = font_get_width(recording->formatted_date)/64.0;
		col_widths[0] = MAX(col_widths[0], width);
		width = font_get_width(recording->formatted_time)/64.0;
		col_widths[1] = MAX(col_widths[1], width);
	}

	col_widths[0] += 2*ConfCellPaddingX;
	col_widths[1] += 2*ConfCellPaddingX;

	font_set_element("column-heading");
	int y = viewport[3] - font_get_height()/64 - ConfCellPaddingY;
	int x = 0;

	set_colour("column-heading-bg", FALSE);
	glBegin(GL_QUADS);
	glVertex2i(0, y - ConfCellPaddingY);
	glVertex2i(ConfScreenWidth, y - ConfCellPaddingY);
	glVertex2i(ConfScreenWidth, y + font_get_height()/64 + ConfCellPaddingY);
	glVertex2i(0, y + font_get_height()/64 + ConfCellPaddingY);
	glEnd();

	set_colour("column-heading-text", FALSE);

	width = font_get_width(COLUMN_DATE)/64;
	x = viewport[0] + col_widths[0] - width - ConfCellPaddingX;
	font_draw(COLUMN_DATE, x, y);

	width = font_get_width(COLUMN_TIME)/64;
	x = viewport[0] + col_widths[0] + col_widths[1] - width - ConfCellPaddingX;
	font_draw(COLUMN_TIME, x, y);

	x = viewport[0] + col_widths[0] + col_widths[1] + ConfCellPaddingX;
	font_draw(COLUMN_TITLE, x, y);

	font_set_element("normal");
	int row_height = font_get_height()/64 + 2*ConfCellPaddingY;
	y -= row_height;

	DisplayRows = 0;
	set_colour("normal", FALSE);

	for(GList *i = Top; i; i = i->next)
	{
		VdrRecording *recording = i->data;

		if(i == Current)
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

		width = font_get_width(recording->formatted_date)/64;
		x = viewport[0] + col_widths[0] - width - ConfCellPaddingX;
		font_draw(recording->formatted_date, x, y);
		width = font_get_width(recording->formatted_time)/64;
		x = viewport[0] + col_widths[0] + col_widths[1] - width - ConfCellPaddingX;
		font_draw(recording->formatted_time, x, y);
		x = viewport[0] + col_widths[0] + col_widths[1] + ConfCellPaddingX;
		font_draw(recording->name, x, y);
		y -= row_height;

		if(y < viewport[1])
			break;

		++DisplayRows;
	}
}

static void on_recordings_up()
{
	if(!Current->prev)
		return;

	if(Current == Top)
		Top = Top->prev;

	Current = Current->prev;

	RecordingsWindow->dirty = TRUE;
}

static void on_recordings_down()
{
	if(!Current->next)
		return;

	Current = Current->next;

	int diff = g_list_position(VdrRecordings, Current)
		- g_list_position(VdrRecordings, Top);

	if(DisplayRows && diff > DisplayRows)
		Top = Top->next;

	RecordingsWindow->dirty = TRUE;
}

static void on_recordings_select()
{
	VdrRecording *recording = Current->data;
	DIR *dir = opendir(recording->path);

	if(!dir)
	{
		g_warning("%s: error opening directory: %s",
				recording->path, strerror(errno));
		return;
	}

	char *path = g_strdup_printf("%d-%02d-%02d.%02d.%02d.",
			g_date_get_year(recording->date),
			g_date_get_month(recording->date),
			g_date_get_day(recording->date),
			recording->time/3600,
			(recording->time % 3600)/60);

	struct dirent *dirent;
	while((dirent = readdir(dir)))
		if(strncmp(dirent->d_name, path, strlen(path)) == 0)
			break;

	if(!dirent || !dirent->d_type & DT_DIR)
	{
		g_warning("%s: can't find recording for: %s",
				recording->path, path);
		g_free(path);
		return;
	}

	g_free(path);
	path = g_strdup_printf("%s/%s/", recording->path, dirent->d_name);

	closedir(dir);

	if(!(dir = opendir(path)))
	{
		g_warning("%s: error opening directory: %s",
				path, strerror(errno));
		g_free(path);
		return;
	}

	GList *files = NULL;

	while((dirent = readdir(dir)))
	{
		char *n = dirent->d_name;

		if(strlen(n) == 7 && isdigit(n[0]) && isdigit(n[1])
				&& isdigit(n[2]) && strcmp(n + 3, ".vdr") == 0)
			files = g_list_append(files, g_strdup(n));
	}

	closedir(dir);

	if(!files)
	{
		g_warning("%s: no recordings", path);
		g_free(path);
		return;
	}

	run_player(path, files);

	g_free(path);

	for(GList *i = files; i; i = i->next)
		g_free(i->data);

	g_list_free(files);
}

static gint recordings_cmp(VdrRecording *a, VdrRecording *b)
{
	gint result;

	if((result = g_date_compare(a->date, b->date)) == 0)
		result = (a->time < b->time ? -1 :
				(a->time > b->time ? 1 : 0));

	result = (result > 0 ? -1 : (result < 0 ? 1 : 0));

	return result;
}

static gboolean info_event(Window *window, Event event)
{
	if(window != InfoWindow)
		return FALSE;

	switch(event)
	{
		case EVENT_INFO:
		case EVENT_BACK:
		case EVENT_SELECT:
			vdr_free_recording_info(Info);
			Info = NULL;
			window_hide(InfoWindow);
			return TRUE;
		default:
			break;
	}

	return FALSE;
}

static void info_expose(Window *window)
{
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	VdrRecording *recording = Current->data;

	font_set_element("dialog-heading");
	int width = MAX(ConfDialogWidth, font_get_width(recording->name)/64);
	int height = 2*font_get_height()/64;

	font_set_element("dialog-body");
	int lines = font_get_wrapped_lines(Info->description, width);
	height += lines*font_get_line_height()/64;

	int x = viewport[0] + (viewport[2] - viewport[0])/2;
	int y = viewport[1] + (viewport[3] - viewport[1])/2;

	font_set_element("dialog-heading");

	set_colour("dialog-heading-bg", TRUE);
	glBegin(GL_QUADS);
	glVertex2i(x - width/2 - ConfDialogMargin, y + height/2 - font_get_line_height()/64);
	glVertex2i(x + width/2 + ConfDialogMargin, y + height/2 - font_get_line_height()/64);
	glVertex2i(x + width/2 + ConfDialogMargin, y + height/2 + ConfDialogMargin);
	glVertex2i(x - width/2 - ConfDialogMargin, y + height/2 + ConfDialogMargin);
	glEnd();

	set_colour("dialog-body-bg", TRUE);
	glBegin(GL_QUADS);
	glVertex2i(x - width/2 - ConfDialogMargin, y - height/2 - ConfDialogMargin);
	glVertex2i(x + width/2 + ConfDialogMargin, y - height/2 - ConfDialogMargin);
	glVertex2i(x + width/2 + ConfDialogMargin,
			y + height/2 - font_get_line_height()/64);
	glVertex2i(x - width/2 - ConfDialogMargin,
			y + height/2 - font_get_line_height()/64);
	glEnd();

	set_colour("dialog-border", TRUE);
	glLineWidth(ConfDialogBorderWidth);
	glBegin(GL_LINE_LOOP);
	glVertex2i(x - width/2 - ConfDialogMargin, y - height/2 - ConfDialogMargin);
	glVertex2i(x + width/2 + ConfDialogMargin, y - height/2 - ConfDialogMargin);
	glVertex2i(x + width/2 + ConfDialogMargin, y + height/2 + ConfDialogMargin);
	glVertex2i(x - width/2 - ConfDialogMargin, y + height/2 + ConfDialogMargin);

	glEnd();

	set_colour("dialog-heading-text", FALSE);

	font_draw(recording->name, x - width/2,
			y + height/2 - font_get_height()/64);

	y += height/2 - 2*font_get_line_height()/64;

	set_colour("dialog-body-text", FALSE);
	font_set_element("dialog-body");
	font_draw_wrapped(Info->description, x - width/2, y, width);

	glPopAttrib();
}

gboolean confirm_delete_event(Window *window, Event event)
{
	if(window != ConfirmDeleteWindow)
		return FALSE;

	switch(event)
	{
		case EVENT_LEFT:
			if(ConfirmDeleteSelection > 0)
			{
				--ConfirmDeleteSelection;
				window->dirty = TRUE;
			}
			return TRUE;
		case EVENT_RIGHT:
			if(ConfirmDeleteSelection < 1)
			{
				++ConfirmDeleteSelection;
				window->dirty = TRUE;
			}
			return TRUE;
		case EVENT_SELECT:
			if(ConfirmDeleteSelection == 0)
			{
				VdrRecording *recording = Current->data;
				if(Top == Current)
					Top = Top->next;
				if(Current)
					Current = Current->next;
				vdr_delete_recording(recording->id);
				recordings_update();
			}
			window_hide(ConfirmDeleteWindow);
			return TRUE;
		case EVENT_BACK:
			window_hide(ConfirmDeleteWindow);
			return TRUE;
		default:
			break;
	}

	return FALSE;
}

static void confirm_delete_expose(Window *window)
{
	VdrRecording *recording = Current->data;

	char *heading = "Confirm Delete";
	char *body = g_strdup_printf("Really delete `%s'?", recording->name);

	int width = ConfDialogWidth;
	int header_height = 0;
	int body_height = 0;
	int buttons_height = 0;

	font_set_element("dialog-heading");
	width = MAX(width, font_get_width(heading)/64);
	header_height = font_get_height()/64 + 2*ConfDialogMargin;

	font_set_element("button");
	int buttons_width = 0;
	buttons_width += 2*ConfButtonMargin;
	buttons_width += 2*ConfButtonPaddingX;
	buttons_width += font_get_width("Yes")/64;
	buttons_width += ConfButtonSpacing;
	buttons_width += 2*ConfButtonMargin;
	buttons_width += 2*ConfButtonPaddingX;
	buttons_width += font_get_height("No")/64;
	width = MAX(width, buttons_width);
	buttons_height = font_get_height()/64;
	buttons_height += ConfButtonMargin;
	buttons_height += 2*ConfButtonPaddingY;

	font_set_element("dialog-body");
	int lines = font_get_wrapped_lines(body, width);
	body_height = font_get_height()/64;
	body_height += (lines - 1)*font_get_line_height()/64;
	body_height += 2*ConfDialogMargin;

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	int x = viewport[0] + (viewport[2] - viewport[0])/2;
	int y = viewport[1] + (viewport[3] - viewport[1])/2;
	int height = header_height + body_height + buttons_height;

	x -= width/2;
	y += height/2;

	set_colour("dialog-heading-bg", TRUE);
	glBegin(GL_QUADS);
	glVertex2i(x - ConfDialogMargin, y - header_height);
	glVertex2i(x + width + ConfDialogMargin, y - header_height);
	glVertex2i(x + width + ConfDialogMargin, y);
	glVertex2i(x - ConfDialogMargin, y);
	glEnd();

	set_colour("dialog-heading-text", FALSE);
	font_set_element("dialog-heading");
	font_draw(heading, x, y - ConfDialogMargin - font_get_height()/64);

	y -= header_height;

	set_colour("dialog-body-bg", TRUE);
	glBegin(GL_QUADS);
	glVertex2i(x - ConfDialogMargin, y - body_height);
	glVertex2i(x + width + ConfDialogMargin, y - body_height);
	glVertex2i(x + width + ConfDialogMargin, y);
	glVertex2i(x - ConfDialogMargin, y);
	glEnd();

	set_colour("dialog-body-text", FALSE);
	font_set_element("dialog-body");
	font_draw_wrapped(body, x,
			y - ConfDialogMargin - font_get_height()/64,
			width);

	y -= body_height;

	set_colour("dialog-button-bg", TRUE);
	glBegin(GL_QUADS);
	glVertex2i(x - ConfDialogMargin, y - buttons_height);
	glVertex2i(x + width + ConfDialogMargin, y - buttons_height);
	glVertex2i(x + width + ConfDialogMargin, y);
	glVertex2i(x - ConfDialogMargin, y);
	glEnd();

	font_set_element("button");
	int button_x = x + width - ConfButtonMargin;
	int button_y = y;
	int button_width = 2*ConfButtonPaddingX + font_get_width("No")/64;
	int button_height = 2*ConfButtonPaddingY + font_get_height()/64;

	if(ConfirmDeleteSelection == 1)
		set_colour("button-active-bg", TRUE);
	else
		set_colour("button-normal-bg", TRUE);

	glBegin(GL_QUADS);
	glVertex2i(button_x - button_width, button_y - button_height);
	glVertex2i(button_x, button_y - button_height);
	glVertex2i(button_x, button_y);
	glVertex2i(button_x - button_width, button_y);
	glEnd();

	if(ConfirmDeleteSelection == 1)
		set_colour("button-active-text", FALSE);
	else
		set_colour("button-normal-text", FALSE);

	font_draw("No", button_x - button_width + ConfButtonPaddingX,
			button_y - button_height + ConfButtonPaddingY);

	button_x -= button_width + ConfButtonSpacing;
	button_width = 2*ConfButtonPaddingX + font_get_width("Yes")/64;

	if(ConfirmDeleteSelection == 0)
		set_colour("button-active-bg", TRUE);
	else
		set_colour("button-normal-bg", TRUE);

	glBegin(GL_QUADS);
	glVertex2i(button_x - button_width, button_y - button_height);
	glVertex2i(button_x, button_y - button_height);
	glVertex2i(button_x, button_y);
	glVertex2i(button_x - button_width, button_y);
	glEnd();

	if(ConfirmDeleteSelection == 0)
		set_colour("button-active-text", FALSE);
	else
		set_colour("button-normal-text", FALSE);

	font_draw("Yes", button_x - button_width + ConfButtonPaddingX,
			button_y - button_height + ConfButtonPaddingY);

	g_free(body);
}

