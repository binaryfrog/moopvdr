#ifndef VDR_H
#define VDR_H

#include <glib.h>

typedef struct _VdrChannel VdrChannel;
typedef struct _VdrRecording VdrRecording;
typedef struct _VdrRecordingInfo VdrRecordingInfo;
typedef struct _VdrEvent VdrEvent;

struct _VdrChannel
{
	int number;

	int group_number;
	char *name;
	char *short_name;
	char *provider;

	int frequency;

	char *parameters;

	char *source;
	int symbol_rate;
	int vpid;
	char *apids;
	int tpid;
	char *cpids;
	int sid;
	int nid;
	int tid;
	int rid;
};

struct _VdrRecording
{
	int id;
	char *name;
	char *path;
	GDate *date;
	int time;
	char *formatted_time;
	char *formatted_date;
};

struct _VdrRecordingInfo
{
	char *description;
};

struct _VdrEvent
{
	unsigned event_id;
	time_t start_time;
	int duration;
	char *title;
	char *description;
	char *formatted_time;
};

void vdr_init(const char *hostname, int port);

GList *vdr_get_channel_list();
void vdr_free_channel_list(GList *channels);

GList *vdr_get_recordings_list();
void vdr_free_recordings_list(GList *recordings);

void vdr_delete_recording(int id);

VdrRecordingInfo *vdr_get_recording_info(int id);
void vdr_free_recording_info(VdrRecordingInfo *info);

GList *vdr_get_events_list(int channel);
void vdr_free_events_list(GList *events);

#endif // not VDR_H
