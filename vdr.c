#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/poll.h>

#include "vdr.h"
#include "conf.h"
#include "util.h"
#include "common.h"

static struct sockaddr_in VdrSockAddr;
static int VdrFd;
static char VdrBuf[4096];
static size_t VdrBufUsed;

static void vdr_open();
static void vdr_close();
static gboolean vdr_poll(int flag);
static void vdr_write(const char *s, size_t size);
static void vdr_writef(const char *format, ...);
static void vdr_read();
static char *vdr_readline();
static char *vdr_exchange_chars(char *name);

void vdr_init(const char *hostname, int port)
{
	struct hostent *hent;

	if((hent = gethostbyname(hostname)) == NULL)
	{
		g_critical("%s: host error: %s", hostname, hstrerror(h_errno));
		exit(1);
	}

	VdrSockAddr.sin_family = AF_INET;
	VdrSockAddr.sin_port = htons(port);
	memcpy(&VdrSockAddr.sin_addr, hent->h_addr_list[0],
			sizeof(struct in_addr));
}

GList *vdr_get_channel_list()
{
	GList *channels = NULL;

	vdr_open();

	vdr_writef("LSTC\n");

	gboolean last = FALSE;
	char *reply;

	while(!last && (reply = vdr_readline()))
	{
		gchar **bits = g_strsplit(reply, ":", 0);
		gchar *ptr, *end;

		int n;
		for(n = 0; bits[n]; ++n)
			;

		if(n != 13)
		{
			g_critical("invalid response: %s", reply);
			exit(1);
		}

		ptr = NULL;
		int code = strtoul(bits[0], &ptr, 10);
		if(code != 250 || !ptr || !*ptr)
		{
			g_critical("invalid response: %s", reply);
			exit(1);
		}

		if(*ptr++ == ' ')
			last = TRUE;

		VdrChannel *channel = g_new0(VdrChannel, 1);
		channel->number = strtoul(ptr, &ptr, 10);
		++ptr;

		for(end = ptr; *end; ++end)
			if(*end == '|')
				*end = ':';

		if((end = strchr(ptr, ';')))
		{
			*end++ = '\0';
			channel->provider = g_strdup(end);
		}

		if((end = strchr(ptr, ',')))
		{
			*end++ = '\0';
			channel->short_name = g_strdup(end);
		}

		channel->name = g_strdup(ptr);

		channel->frequency = strtoul(bits[1], NULL, 10);
		channel->parameters = g_strdup(bits[2]);
		channel->source = g_strdup(bits[3]);
		channel->symbol_rate = strtoul(bits[4], NULL, 10);
		channel->vpid = strtoul(bits[5], NULL, 10);
		channel->apids = g_strdup(bits[6]);
		channel->tpid = strtoul(bits[7], NULL, 10);
		channel->cpids = g_strdup(bits[8]);
		channel->sid = strtoul(bits[9], NULL, 10);
		channel->nid = strtoul(bits[10], NULL, 10);
		channel->tid = strtoul(bits[11], NULL, 10);
		channel->rid = strtoul(bits[12], NULL, 10);

		channels = g_list_append(channels, channel);

		g_strfreev(bits);
		g_free(reply);
	}

	vdr_close();

	return channels;
}

void vdr_free_channel_list(GList *list)
{
}

GList *vdr_get_recordings_list()
{
	GList *recordings = NULL;

	vdr_open();

	vdr_writef("LSTR\n");

	gboolean last = FALSE;
	char *reply;

	while(!last && (reply = vdr_readline()))
	{
		if(strncmp(reply, "250", 3) != 0)
		{
			g_critical("invalid response: %s", reply);
			exit(1);
		}

		if(reply[3] == ' ')
			last = TRUE;

		gchar **bits = g_strsplit(reply + 4, " ", 4);

		VdrRecording *recording = g_new0(VdrRecording, 1);
		recording->id = strtoul(bits[0], NULL, 10);
		recording->name = g_strdup(bits[3]);

		char *tmp = g_malloc0(strlen(recording->name)*3);
		strcpy(tmp, recording->name);
		vdr_exchange_chars(tmp);
		recording->path = g_strdup_printf("%s/%s",
				ConfVdrVideoDir, tmp);
		g_free(tmp);

		int day = strtoul(bits[1], NULL, 10);
		int month = strtoul(bits[1] + 3, NULL, 10);
		int year = strtoul(bits[1] + 6, NULL, 10);
		year += (year >= 70 ? 1900 : 2000);
		recording->date = g_date_new_dmy(day, month, year);

		int hour = strtoul(bits[2], NULL, 10);
		int min = strtoul(bits[2] + 3, NULL, 10);
		recording->time = hour*3600 + min*60;

		recording->formatted_date = get_formatted_date(
				recording->date);
		recording->formatted_time = g_strdup_printf("%02d:%02d",
				hour, min);

		recordings = g_list_append(recordings, recording);

		g_strfreev(bits);
		g_free(reply);
	}

	vdr_close();

	return recordings;
}

void vdr_free_recordings_list(GList *recordings)
{
	for(GList *i = recordings; i; i = i->next)
	{
		VdrRecording *recording = i->data;

		if(recording->name)
			g_free(recording->name);
		if(recording->path)
			g_free(recording->path);
		if(recording->date)
			g_date_free(recording->date);
		if(recording->formatted_time)
			g_free(recording->formatted_time);
		if(recording->formatted_date)
			g_free(recording->formatted_date);

		g_free(recording);
	}

	g_list_free(recordings);
}

void vdr_delete_recording(int id)
{
	vdr_open();
	vdr_writef("DELR %d\n", id);

	char *reply = vdr_readline();
	g_free(reply);

	vdr_close();
}

VdrRecordingInfo *vdr_get_recording_info(int id)
{
	vdr_open();

	vdr_writef("LSTR %d\n", id);

	VdrRecordingInfo *info = g_new0(VdrRecordingInfo, 1);

	gboolean last = FALSE;
	char *reply;

	while(!last && (reply = vdr_readline()))
	{
		if(strncmp(reply, "215", 3) != 0)
		{
			g_critical("invalid response: %s", reply);
			exit(1);
		}

		if(reply[3] == ' ')
			last = TRUE;

		if(reply[4] == 'D')
			info->description = g_strdup(reply + 6);
		
		g_free(reply);
	}

	vdr_close();

	return info;
}

void vdr_free_recording_info(VdrRecordingInfo *info)
{
	if(info->description)
		g_free(info->description);

	g_free(info);
}

GList *vdr_get_events_list(int channel)
{
	vdr_open();

	vdr_writef("LSTE %d\n", channel);

	GList *events = NULL;
	VdrEvent *event = NULL;

	char *reply;

	while((reply = vdr_readline()))
	{
		if(strncmp(reply, "215", 3) != 0)
		{
			g_critical("invalid response: %s", reply);
			exit(1);
		}

		if(reply[3] == ' ')
			break;

		if(reply[4] == 'E')
		{
			event = g_new0(VdrEvent, 1);

			gchar **bits = g_strsplit(reply + 6, " ", 4);
			event->event_id = strtoul(bits[0], NULL, 10);
			event->start_time = strtol(bits[1], NULL, 10);
			event->duration = strtol(bits[2], NULL, 10);

			time_t t = event->start_time;
			struct tm *tm = localtime(&t);
			char *s = g_malloc(64);
			strftime(s, 64, "%l:%M %P", tm);
			event->formatted_time = s;

			g_strfreev(bits);
			events = g_list_append(events, event);
		} else if(reply[4] == 'T' && event)
			event->title = g_strdup(reply + 6);
		else if(reply[4] == 'D' && event)
			event->description = g_strdup(reply + 6);

		g_free(reply);
	}

	vdr_close();

	return events;
}

void vdr_free_events_list(GList *events)
{
	for(GList *i = events; i; i = i->next)
	{
		VdrEvent *event = i->data;

		if(event->title)
			g_free(event->title);
		if(event->description)
			g_free(event->description);

		g_free(event);
	}

	g_list_free(events);
}

static void vdr_open()
{
	if((VdrFd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	{
		g_critical("socket error: %s", strerror(errno));
		exit(1);
	}

	if(connect(VdrFd, (struct sockaddr *) &VdrSockAddr,
				sizeof(struct sockaddr_in)) != 0)
	{
		g_critical("connect error: %s", strerror(errno));
		exit(1);
	}

	char *reply = vdr_readline();
	if(strncmp(reply, "220", 3) != 0)
	{
		g_critical("invalid vdr response: %s", reply);
		exit(1);
	}
}

static void vdr_close()
{
	close(VdrFd);
	VdrFd = -1;
	VdrBufUsed = 0;
}

static gboolean vdr_poll(int flags)
{
	struct pollfd pollset = { VdrFd, flags, 0 };
	int result = poll(&pollset, 1, 1000);

	if(result == -1)
	{
		g_critical("poll error: %s", strerror(errno));
		exit(1);
	}

	if(result == 1)
	{
		if(pollset.revents & (POLLHUP | POLLERR))
		{
			g_critical("connection hangup or error");
			exit(1);
		}

		if(pollset.revents & flags)
			return TRUE;
	}

	return FALSE;
}

static void vdr_write(const char *s, size_t size)
{
	for(;;)
	{
		if(vdr_poll(POLLOUT))
		{
			if(write(VdrFd, s, size) != size)
			{
				g_critical("short write: %s", strerror(errno));
				exit(1);
			}

			break;
		}
	}
}

static void vdr_writef(const char *format, ...)
{
	char buf[1024];
	va_list args;

	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);

	vdr_write(buf, strlen(buf));
}

static void vdr_read()
{
	if(VdrBufUsed == sizeof(VdrBuf))
	{
		g_critical("read buffer overflow");
		exit(1);
	}

	int n = read(VdrFd, VdrBuf + VdrBufUsed, sizeof(VdrBuf) - VdrBufUsed);

	if(n < 0)
	{
		g_critical("read error: %s", strerror(errno));
		exit(1);
	} else if(n == 0)
	{
		g_critical("connection reset by peer");
		exit(1);
	}

	VdrBufUsed += n;
}

static char *vdr_readline()
{
	char *ptr;

	for(;;)
	{
		if(!(ptr = memchr(VdrBuf, '\n', VdrBufUsed)))
		{
			if(vdr_poll(POLLIN))
				vdr_read();

			continue;
		}

		if(ptr == VdrBuf)
		{
			while(ptr < VdrBuf + VdrBufUsed && *ptr == '\n')
				++ptr;

			memmove(VdrBuf, ptr, VdrBufUsed - (ptr - VdrBuf));
			VdrBufUsed -= (ptr - VdrBuf);
			continue;
		}

		char *line = g_malloc((ptr - VdrBuf) + 1);
		memcpy(line, VdrBuf, ptr - VdrBuf);
		line[ptr - VdrBuf] = '\0';

		if(line[strlen(line) - 1] == '\r')
			line[strlen(line) - 1] = '\0';

		while(ptr < VdrBuf + VdrBufUsed && *ptr == '\n')
			++ptr;

		memmove(VdrBuf, ptr, VdrBufUsed - (ptr - VdrBuf));
		VdrBufUsed -= (ptr - VdrBuf);

		return line;
	}
}

// Taken from vdr/recording.c
static char *vdr_exchange_chars(char *s)
{
	for(char *p = s; *p; ++p)
	{
		switch (*p) {
			// characters that can be used "as is":
			case '!':
			case '@':
			case '$':
			case '%':
			case '&':
			case '(':
			case ')':
			case '+':
			case ',':
			case '-':
			case ';':
			case '=':
			case '0' ... '9':
			case 'a' ... 'z':
			case 'A' ... 'Z':
			/*case 'ä': case 'Ä':
			case 'ö': case 'Ö':
			case 'ü': case 'Ü':
			case 'ß':*/
				break;
			// characters that can be mapped to other characters:
			case ' ': *p = '_'; break;
			case '~': *p = '/'; break;
			// characters that have to be encoded:
			default:
				if (*p != '.' || !*(p + 1) || *(p + 1) == '~')
				{
					// Windows can't handle '.' at the end of directory names
					int l = p - s;
					/*s = (char *)realloc(s, strlen(s) + 10);*/
					p = s + l;
					char buf[4];
					sprintf(buf, "#%02X", (unsigned char)*p);
					memmove(p + 2, p, strlen(p) + 1);
					strncpy(p, buf, 3);
					p += 2;
				}
		}
	}

	return s;
}

