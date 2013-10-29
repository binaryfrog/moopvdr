#ifndef EVENT_H
#define EVENT_H

#include <sys/types.h>

typedef enum _Event Event;

enum _Event
{
	EVENT_NONE,
	EVENT_EXIT,
	EVENT_CHILD_EXITED,
	EVENT_EXPOSE,
	EVENT_MAPPED,
	EVENT_UNMAPPED,
	EVENT_DIALOG_RESPONSE,
	EVENT_SELECT,
	EVENT_BACK,
	EVENT_UP,
	EVENT_DOWN,
	EVENT_LEFT,
	EVENT_RIGHT,
	EVENT_INFO,
	EVENT_RED,
	EVENT_GREEN,
	EVENT_YELLOW,
	EVENT_BLUE,
	EVENT_CHANNEL_UP,
	EVENT_CHANNEL_DOWN
};

extern pid_t EventChildPid;
extern int EventChildStatus;

void event_init();
Event event_poll();

#endif // not EVENT_H
