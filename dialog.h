#ifndef DIALOG_H
#define DIALOG_H

#include <glib.h>

#include "window.h"

typedef enum _DialogResponse DialogResponse;
typedef struct _DialogButton DialogButton;
typedef struct _Dialog Dialog;

enum _DialogResponse
{
	DIALOG_RESPONSE_NONE,
	DIALOG_RESPONSE_OK,
	DIALOG_RESPONSE_CANCEL,
	DIALOG_RESPONSE_CLOSE,
	DIALOG_RESPONSE_YES,
	DIALOG_RESPONSE_NO
};

struct _DialogButton
{
	char *label;
	int response;
};

struct _Dialog
{
	Window *window;
	char *header;
	char *body;
	GList *buttons;
	int active_button;
	int response;
};

Dialog *dialog_new(const char *header, const char *body);
void dialog_delete(Dialog *dialog);

void dialog_show(Dialog *dialog);

void dialog_set_header(Dialog *dialog, const char *header);
void dialog_set_body(Dialog *dialog, const char *body);

#endif // not DIALOG_H
