#include <glib.h>

#include "internal.h"

#include "debug.h"
#include <prefs.h>


#include "gtkntf_event.h"
#include "gtkntf_event_info.h"

struct _GtkNtfEventInfo {
	GtkNtfEvent *event;

	guint timeout_id;

	gchar *source;
	gchar *title;
	gchar *content;

	GCallback open_action;
	
};


/*******************************************************************************
 * API
 ******************************************************************************/
static void
gtkntf_event_info_free_string(gchar *string) {
	if(string) {
		g_free(string);
		string = NULL;
	}
}

GtkNtfEventInfo *
gtkntf_event_info_new(const gchar *notification_type) {
	GtkNtfEvent *event;
	GtkNtfEventInfo *info;

	g_return_val_if_fail(notification_type, NULL);

	event = gtkntf_event_find_for_notification(notification_type);
	g_return_val_if_fail(event, NULL);

	info = g_new0(GtkNtfEventInfo, 1);
	info->event = event;

	info->source  = NULL;
	info->title   = NULL;
	info->content = NULL;

	return info;
}

void
gtkntf_event_info_destroy(GtkNtfEventInfo *info) {
	g_return_if_fail(info);

	info->event 	= NULL;

	gtkntf_event_info_free_string(info->source);

	gtkntf_event_info_free_string(info->title);

	gtkntf_event_info_free_string(info->content);

	if(info->timeout_id)
		g_source_remove(info->timeout_id);

	g_free(info);
	info = NULL;
}

GtkNtfEvent *
gtkntf_event_info_get_event(GtkNtfEventInfo *info) {
	g_return_val_if_fail(info, GTKNTF_EVENT_TYPE_UNKNOWN);

	return info->event;
}

void
gtkntf_event_info_set_source(GtkNtfEventInfo *info, const gchar *source) {
	g_return_if_fail(info);

	gtkntf_event_info_free_string(info->source);

	info->source = g_strdup(source);
}

void
gtkntf_event_info_set_content(GtkNtfEventInfo *info, const gchar *content) {
	g_return_if_fail(info);
	g_return_if_fail(content);

	gtkntf_event_info_free_string(info->content);

	info->content = g_strdup(content);
}

const gchar *
gtkntf_event_info_get_content(GtkNtfEventInfo *info) {
	g_return_val_if_fail(info, NULL);

	return info->content;
}

const gchar *
gtkntf_event_info_get_source(GtkNtfEventInfo *info) {
	g_return_val_if_fail(info, NULL);

	return info->source;
}

void
gtkntf_event_info_set_title(GtkNtfEventInfo *info, const gchar *title) {
	g_return_if_fail(info);
	g_return_if_fail(title);

	gtkntf_event_info_free_string(info->title);
	
	info->title = g_strdup(title);
}

const gchar *
gtkntf_event_info_get_title(GtkNtfEventInfo *info) {
	g_return_val_if_fail(info, NULL);

	return info->title;
}


void
gtkntf_event_info_set_timeout_id(GtkNtfEventInfo *info, guint timeout_id) {
	g_return_if_fail(info);

	info->timeout_id = timeout_id;
}

guint
gtkntf_event_info_get_timeout_id(GtkNtfEventInfo *info) {
	g_return_val_if_fail(info, -1);

	return info->timeout_id;
}

void
gtkntf_event_info_set_open_action(GtkNtfEventInfo *info, GCallback open_action) {
	g_return_if_fail(info);

	info->open_action = open_action;
}

GCallback
gtkntf_event_info_get_open_action(const GtkNtfEventInfo *info) {
	g_return_val_if_fail(info, NULL);

	return info->open_action;
}
