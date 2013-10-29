#ifndef UTIL_H
#define UTIL_H

#include <time.h>
#include <glib.h>
#include <sys/types.h>

char *get_formatted_date(GDate *date);
char *get_formatted_date_time(GDate *date, int time);

void set_colour(const char *name, gboolean opacity);

pid_t run_player(const char *dir, GList *files);

#endif // not UTIL_H
