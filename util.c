#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <glib.h>
#include <GL/gl.h>

#include "util.h"
#include "conf.h"

char *get_formatted_date(GDate *date)
{
	GDate *today = g_date_new();
	g_date_set_time_t(today, time(NULL));

	int days_diff = g_date_days_between(date, today);
	g_date_free(today);

	char result[32];

	if(days_diff == 0)
		return g_strdup("Today");
	else if(days_diff == 1)
		return g_strdup("Yesterday");
	else if(days_diff < 7)
		g_date_strftime(result, sizeof(result), "%A", date);
	else
		g_date_strftime(result, sizeof(result), "%d/%m/%Y", date);

	return g_strdup(result);
}

char *get_formatted_date_time(GDate *date, int secs)
{
	GDate *today = g_date_new();
	g_date_set_time_t(today, time(NULL));

	int days_diff = g_date_days_between(date, today);

	char date_str[32];

	if(days_diff < 7)
	{
		if(days_diff == 0)
			strcpy(date_str, "Today");
		else if(days_diff == 1)
			strcpy(date_str, "Yesterday");
		else
			g_date_strftime(date_str, sizeof(date_str), "%A",
					date);
	} else
		g_date_strftime(date_str, sizeof(date_str), "%d/%m/%Y", date);

	return g_strdup_printf("%s %02.0f:%02.0f", date_str,
			floor(secs/3600.0), floor((secs % 3600)/60.0));
}

void set_colour(const char *name, gboolean opacity)
{
	GList *i;
	for(i = ConfColours; i; i = i->next)
		if(strcmp(((ConfColour *) i->data)->element, name) == 0)
			break;

	if(i)
	{
		ConfColour *cc = i->data;

		if(opacity)
			glColor4ub(cc->rgba[0], cc->rgba[1],
					cc->rgba[2], cc->rgba[3]);
		else
			glColor4ub(cc->rgba[0], cc->rgba[1],
					cc->rgba[2], 0xFF);
	}
}

pid_t run_player(const char *dir, GList *files)
{
	pid_t pid = fork();

	if(pid == 0)
	{
		char **args;
		int nargs = 1;

		for(int i = 0; ConfPlayerArguments[i]; ++i)
			++nargs;

		nargs += g_list_length(files);

		args = g_new0(char *, nargs + 1);
		nargs = 1;

		for(int i = 0; ConfPlayerArguments[i]; ++i)
			args[nargs++] = ConfPlayerArguments[i];

		for(GList *i = files; i; i = i->next)
			args[nargs++] = i->data;

		args[0] = ConfPlayerProgram;
		args[nargs] = NULL;

		chdir(dir);
		execv(ConfPlayerProgram, args);
	}

	return pid;
}

