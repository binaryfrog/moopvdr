#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "conf.h"

typedef enum _ConfType ConfType;
typedef struct _ConfItem ConfItem;

enum _ConfType
{
	CONF_TYPE_STRING,
	CONF_TYPE_STRING_LIST,
	CONF_TYPE_INTEGER,
	CONF_TYPE_DOUBLE,
	CONF_TYPE_BOOLEAN
};

struct _ConfItem
{
	char *group;
	char *key;
	ConfType type;
	void *value;
	int set:1;
};

char *ConfVdrHostname;
int ConfVdrPort;
char *ConfVdrVideoDir;

int ConfScreenWidth;
int ConfScreenHeight;
int ConfScreenMarginX;
int ConfScreenMarginY;
gboolean ConfScreenFullscreen;
gboolean ConfScreenHideCursor;

int ConfCellPaddingX;
int ConfCellPaddingY;

int ConfHeadingPaddingY;
int ConfFooterPaddingY;

int ConfDialogMargin;
int ConfDialogWidth;
int ConfDialogBorderWidth;

int ConfButtonMargin;
int ConfButtonPaddingX;
int ConfButtonPaddingY;
int ConfButtonSpacing;

char *ConfFontDefault;
double ConfFontDefaultSize;
gboolean ConfFontShadow;
double ConfFontShadowSize;

char *ConfPlayerProgram;
char **ConfPlayerArguments;

GList *ConfFonts;
GList *ConfColours;

static ConfItem ConfItems[] =
{
	{ "vdr", "hostname", CONF_TYPE_STRING, &ConfVdrHostname, 0 },
	{ "vdr", "port", CONF_TYPE_INTEGER, &ConfVdrPort, 0 },
	{ "vdr", "video-dir", CONF_TYPE_STRING, &ConfVdrVideoDir, 0 },
	{ "screen", "width", CONF_TYPE_INTEGER, &ConfScreenWidth, 0 },
	{ "screen", "height", CONF_TYPE_INTEGER, &ConfScreenHeight, 0 },
	{ "screen", "margin-x", CONF_TYPE_INTEGER, &ConfScreenMarginX, 0 },
	{ "screen", "margin-y", CONF_TYPE_INTEGER, &ConfScreenMarginY, 0 },
	{ "screen", "fullscreen", CONF_TYPE_BOOLEAN, &ConfScreenFullscreen, 0 },
	{ "screen", "hide-cursor", CONF_TYPE_BOOLEAN, &ConfScreenHideCursor, 0 },
	{ "cell", "padding-x", CONF_TYPE_INTEGER, &ConfCellPaddingX, 0 },
	{ "cell", "padding-y", CONF_TYPE_INTEGER, &ConfCellPaddingY, 0 },
	{ "heading", "padding-y", CONF_TYPE_INTEGER, &ConfHeadingPaddingY, 0 },
	{ "footer", "padding-y", CONF_TYPE_INTEGER, &ConfFooterPaddingY, 0 },
	{ "dialog", "margin", CONF_TYPE_INTEGER, &ConfDialogMargin, 0 },
	{ "dialog", "width", CONF_TYPE_INTEGER, &ConfDialogWidth, 0 },
	{ "dialog", "border-width", CONF_TYPE_INTEGER, &ConfDialogBorderWidth, 0 },
	{ "button", "margin", CONF_TYPE_INTEGER, &ConfButtonMargin, 0 },
	{ "button", "padding-x", CONF_TYPE_INTEGER, &ConfButtonPaddingX, 0 },
	{ "button", "padding-y", CONF_TYPE_INTEGER, &ConfButtonPaddingY, 0 },
	{ "button", "spacing", CONF_TYPE_INTEGER, &ConfButtonSpacing, 0 },
	{ "font", "default", CONF_TYPE_STRING, &ConfFontDefault, 0 },
	{ "font", "default-size", CONF_TYPE_DOUBLE, &ConfFontDefaultSize, 0 },
	{ "font", "shadow", CONF_TYPE_BOOLEAN, &ConfFontShadow, 0 },
	{ "font", "shadow-size", CONF_TYPE_DOUBLE, &ConfFontShadowSize, 0 },
	{ "player", "program", CONF_TYPE_STRING, &ConfPlayerProgram, 0 },
	{ "player", "arguments", CONF_TYPE_STRING_LIST, &ConfPlayerArguments, 0 },
	{  NULL, NULL, 0, NULL, 0 }
};

static const char *FileName;
static int FileErrors;

static void conf_get_fonts(GKeyFile *key_file);
static void conf_get_colours(GKeyFile *key_file);
static void conf_set_string(GKeyFile *key_file, ConfItem *conf_item);
static void conf_set_string_list(GKeyFile *key_file, ConfItem *conf_item);
static void conf_set_integer(GKeyFile *key_file, ConfItem *conf_item);
static void conf_set_double(GKeyFile *key_file, ConfItem *conf_item);
static void conf_set_boolean(GKeyFile *key_file, ConfItem *conf_item);

void conf_read(const char *filename)
{
	g_debug("Using configuration file: %s", filename);

	GKeyFile *key_file = g_key_file_new();
	GError *error = NULL;

	if(!g_key_file_load_from_file(key_file, filename, 0, &error))
	{
		g_critical("%s: configuration file error: %s",
				filename, error->message);
		exit(1);
	}

	FileName = filename;
	FileErrors = 0;

	int i;
	for(i = 0; ConfItems[i].group; ++i)
	{
		error = NULL;

		if(!g_key_file_has_key(key_file, ConfItems[i].group,
					ConfItems[i].key, &error))
		{
			g_critical("%s: required key %s.%s not defined "
					"in configuration file", filename,
					ConfItems[i].group, ConfItems[i].key);
			++FileErrors;
			continue;
		}

		switch(ConfItems[i].type)
		{
			case CONF_TYPE_STRING:
				conf_set_string(key_file, &ConfItems[i]);
				break;
			case CONF_TYPE_STRING_LIST:
				conf_set_string_list(key_file, &ConfItems[i]);
				break;
			case CONF_TYPE_INTEGER:
				conf_set_integer(key_file, &ConfItems[i]);
				break;
			case CONF_TYPE_DOUBLE:
				conf_set_double(key_file, &ConfItems[i]);
				break;
			case CONF_TYPE_BOOLEAN:
				conf_set_boolean(key_file, &ConfItems[i]);
				break;
			default:
				g_assert_not_reached();
		}
	}

	conf_get_fonts(key_file);
	conf_get_colours(key_file);

	if(FileErrors)
	{
		g_critical("%s: %d configuration errors",
				filename, FileErrors);
		exit(1);
	}

	g_key_file_free(key_file);

	g_debug("Successfully parsed %d configuration items\n", i);

	return;
}

static void conf_get_fonts(GKeyFile *key_file)
{
	if(!g_key_file_has_group(key_file, "fonts"))
		return;

	GError *error = NULL;

	gsize keys_size;
	gchar **keys = g_key_file_get_keys(key_file, "fonts",
			&keys_size, &error);

	if(error)
	{
		g_critical("%s: error reading group fonts: %s",
				FileName, error->message);
		++FileErrors;
		return;
	}

	for(int i = 0; i < keys_size; ++i)
	{
		ConfFont *conf_font = NULL;

		if(strlen(keys[i]) > 5
				&& strcmp(keys[i] + strlen(keys[i]) - 5,
					"-size") == 0)
		{
			char *element = g_strdup(keys[i]);
			element[strlen(element) - 5] = '\0';

			GList *j;
			for(j = ConfFonts; j; j = j->next)
			{
				conf_font = j->data;
				if(strcmp(conf_font->element, element) == 0)
					break;
			}

			if(!j)
			{
				conf_font = g_new0(ConfFont, 1);
				conf_font->element = element;
				ConfFonts = g_list_append(ConfFonts,
						conf_font);
			}

			error = NULL;
			conf_font->size = g_key_file_get_double(key_file,
					"fonts", keys[i], &error);

			if(error)
			{
				g_critical("%s: invalid font size value "
						"for %s.%s", FileName,
						"fonts", keys[i]);
				++FileErrors;
				continue;
			}
		} else
		{
			GList *j;
			for(j = ConfFonts; j; j = j->next)
			{
				conf_font = j->data;
				if(strcmp(conf_font->element, keys[i]) == 0)
					break;
			}

			if(!j)
			{
				conf_font = g_new0(ConfFont, 1);
				conf_font->element = g_strdup(keys[i]);
				ConfFonts = g_list_append(ConfFonts,
						conf_font);
			}

			error = NULL;
			conf_font->filename = g_key_file_get_string(key_file,
					"fonts", keys[i], &error);
			if(error)
			{
				g_critical("%s: invalid font filename for "
						"%s.%s", FileName, "fonts",
						keys[i]);
				++FileErrors;
				continue;
			}
		}
	}

	for(GList *j = ConfFonts; j; j = j->next)
	{
		ConfFont *conf_font = j->data;
		g_debug("fonts: element=%s filename=%s size=%lf",
				conf_font->element,
				conf_font->filename,
				conf_font->size);
	}
}

static void conf_get_colours(GKeyFile *key_file)
{
	if(!g_key_file_has_group(key_file, "colours"))
		return;

	GError *error = NULL;

	gsize keys_size;
	gchar **keys = g_key_file_get_keys(key_file, "colours",
			&keys_size, &error);

	if(error)
	{
		g_critical("%s: error reading group colours: %s",
				FileName, error->message);
		++FileErrors;
		return;
	}

	for(int i = 0; i < keys_size; ++i)
	{
		char *value = g_key_file_get_string(key_file, "colours",
				keys[i], &error);

		if(error)
		{
			g_critical("%s: invalid colour value for %s.%s",
					FileName, "colours", keys[i]);
			++FileErrors;
			continue;
		}

		gboolean valid = TRUE;

		if(valid && *value != '#')
			valid = FALSE;

		if(valid && strlen(value) != 7 && strlen(value) != 9)
			valid = FALSE;

		if(valid)
			for(char *c = value + 1; valid && *c; ++c)
				valid = isxdigit(*c);

		if(!valid)
		{
			g_critical("%s: invalid colour value for %s.%s",
					FileName, "colours", keys[i]);
			++FileErrors;
			continue;
		}

		ConfColour *conf_colour = g_new0(ConfColour, 1);
		conf_colour->element = g_strdup(keys[i]);

		for(int i = 0; i < 4 && 2*i + 1 < strlen(value); ++i)
		{
			char digits[3] = { 0, 0, 0 };
			digits[0] = value[2*i + 1];
			digits[1] = value[2*i + 2];
			sscanf(digits, "%x", (int *) &conf_colour->rgba[i]);
		}

		ConfColours = g_list_append(ConfColours, conf_colour);
	}

	for(GList *i = ConfColours; i; i = i->next)
	{
		ConfColour *conf_colour = i->data;
		printf("%s: #%02x%02x%02x (%02x)\n", conf_colour->element,
				conf_colour->rgba[0],
				conf_colour->rgba[1],
				conf_colour->rgba[2],
				conf_colour->rgba[3]);
	}
}

static void conf_set_string(GKeyFile *key_file, ConfItem *conf_item)
{
	GError *error = NULL;
	char **conf_value = conf_item->value;
	*conf_value = g_key_file_get_string(key_file, conf_item->group,
			conf_item->key, &error);

	if(error)
	{
		g_critical("%s: invalid string value for configuration "
				"item %s.%s", FileName, conf_item->group,
				conf_item->key);
		++FileErrors;
	}
}

static void conf_set_string_list(GKeyFile *key_file, ConfItem *conf_item)
{
	GError *error = NULL;
	char ***conf_value = conf_item->value;
	*conf_value = g_key_file_get_string_list(key_file, conf_item->group,
			conf_item->key, NULL, &error);

	if(error)
	{
		g_critical("%s: invalid string list value for configuration "
				"item %s.%s", FileName, conf_item->group,
				conf_item->key);
		++FileErrors;
	}
}

static void conf_set_integer(GKeyFile *key_file, ConfItem *conf_item)
{
	GError *error = NULL;
	int *conf_value = conf_item->value;
	*conf_value = g_key_file_get_integer(key_file, conf_item->group,
			conf_item->key, &error);

	if(error)
	{
		g_critical("%s: invalid integer value for configuration "
				"item %s.%s", FileName, conf_item->group,
				conf_item->key);
		++FileErrors;
	}
}

static void conf_set_double(GKeyFile *key_file, ConfItem *conf_item)
{
	GError *error = NULL;
	double *conf_value = conf_item->value;
	*conf_value = g_key_file_get_double(key_file, conf_item->group,
			conf_item->key, &error);

	if(error)
	{
		g_critical("%s: invalid double value for configuration "
				"item %s.%s", FileName, conf_item->group,
				conf_item->key);
		++FileErrors;
	}
}

static void conf_set_boolean(GKeyFile *key_file, ConfItem *conf_item)
{
	GError *error = NULL;
	gboolean *conf_value = conf_item->value;
	*conf_value = g_key_file_get_boolean(key_file, conf_item->group,
			conf_item->key, &error);

	if(error)
	{
		g_critical("%s: invalid boolean value for configuration "
				"item %s.%s", FileName, conf_item->group,
				conf_item->key);
		++FileErrors;
	}
}

