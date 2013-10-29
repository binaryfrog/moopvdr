#ifndef CONF_H
#define CONF_H

#include <glib.h>

typedef struct _ConfFont ConfFont;
typedef struct _ConfColour ConfColour;

struct _ConfFont
{
	char *element;
	char *filename;
	double size;
};

struct _ConfColour
{
	char *element;
	unsigned char rgba[4];
};

extern char *ConfVdrHostname;
extern int ConfVdrPort;
extern char *ConfVdrVideoDir;
extern int ConfScreenHeight;
extern int ConfScreenWidth;
extern int ConfScreenMarginX;
extern int ConfScreenMarginY;
extern gboolean ConfScreenFullscreen;
extern gboolean ConfScreenHideCursor;
extern int ConfCellPaddingX;
extern int ConfCellPaddingY;
extern int ConfHeadingPaddingY;
extern int ConfFooterPaddingY;
extern int ConfDialogMargin;
extern int ConfDialogWidth;
extern int ConfDialogBorderWidth;
extern int ConfButtonMargin;
extern int ConfButtonPaddingX;
extern int ConfButtonPaddingY;
extern int ConfButtonSpacing;
extern char *ConfFontDefault;
extern double ConfFontDefaultSize;
extern gboolean ConfFontShadow;
extern double ConfFontShadowSize;
extern char *ConfPlayerProgram;
extern char **ConfPlayerArguments;

extern GList *ConfFonts;
extern GList *ConfColours;

void conf_read(const char *filename);

#endif // not CONF_H
