#include <string.h>
#include <glib.h>

#include "misc.h"

char *join_paths(const char *p1, const char *p2)
{
	char *ret = g_malloc(strlen(p1) + strlen(p2) + 2);

	strcpy(ret, p1);

	switch((p1[strlen(p1) - 1] == '/') | ((p2[0] == '/') << 1))
	{
		case 0:
			/* P1 doesn't end slash, P2 doesn't begin slash */
			ret[strlen(ret) + 1] = '\0';
			ret[strlen(ret)] = '/';
		case 1:
			/* P1 ends slash, P2 doesn't begin slash */
		case 2:
			/* P1 doesn't end slash, P2 begins slash */
			strcpy(&ret[strlen(ret)], p2);
			break;
		default:
			/* P1 ends slash, P2 begins slash */
			strcpy(&ret[strlen(ret)], &p2[1]);
			break;
	}

	return ret;
}

char *pretty_size(double s)
{
	s /= 1000.0;
	return g_strdup_printf("%.1f GB", s);
}

