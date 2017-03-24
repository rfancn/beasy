#include <glib.h>
#include <string.h>

#include "internal.h"

#include "debug.h"
#include <notify.h>
#include <plugin.h>
#include <prefs.h>
#include <status.h>
#include "util.h"
#include <version.h>

#include "gtkntf.h"
#include "gtkntf_display.h"
#include "gtkntf_event.h"
#include "gtkntf_notification.h"
#include "gtkpref_notify.h"

struct _GtkNtfEvent {
	gchar *n_type;
	gchar *name;
	gchar *description;

	GtkNtfEventPriority priority;

	gchar *tokens;

	gboolean show;
};

static GList *events = NULL;


/*******************************************************************************
 * API
 ******************************************************************************/
GtkNtfEvent *
gtkntf_event_new(const gchar *notification_type, const gchar *tokens,
			 const gchar *name, const gchar *description,
			 GtkNtfEventPriority priority)
{
	GtkNtfEvent *event;

	g_return_val_if_fail(notification_type, NULL);
	g_return_val_if_fail(name, NULL);
	g_return_val_if_fail(description, NULL);

	event = g_new0(GtkNtfEvent, 1);

	event->priority = priority;
	event->n_type 	= g_strdup(notification_type);

	if(tokens)
		event->tokens = g_strdup(tokens);
	else
		event->tokens = g_strdup(TOKENS_DEFAULT);

	event->name = g_strdup(name);
	event->description = g_strdup(description);

	if(!g_list_find(events, event))
		events = g_list_append(events, event);
	else {
		oul_debug_info("GTKNotify", "Event already exists\n");
		gtkntf_event_destroy(event);
	}

	return event;
}

GtkNtfEvent *
gtkntf_event_find_for_notification(const gchar *type) {
	GtkNtfEvent *event;
	GList *l;

	for(l = events; l; l = l->next) {
		event = GTKNTF_EVENT(l->data);
		if(!g_ascii_strcasecmp(event->n_type, type))
			return event;
	}

	return NULL;
}

void
gtkntf_event_destroy(GtkNtfEvent *event) {
	g_return_if_fail(event);

	events = g_list_remove(events, event);

	g_free(event->n_type);
	g_free(event->name);
	g_free(event->description);

	g_free(event);
}

const gchar *
gtkntf_event_get_notification_type(GtkNtfEvent *event) {
	g_return_val_if_fail(event, NULL);

	return event->n_type;
}

const gchar *
gtkntf_event_get_tokens(GtkNtfEvent *event) {
	g_return_val_if_fail(event, NULL);

	return event->tokens;
}

const gchar *
gtkntf_event_get_name(GtkNtfEvent *event) {
	g_return_val_if_fail(event, NULL);

	return event->name;
}

const gchar *
gtkntf_event_get_description(GtkNtfEvent *event) {
	g_return_val_if_fail(event, NULL);

	return event->description;
}

GtkNtfEventPriority
gtkntf_event_get_priority(GtkNtfEvent *event) {
	g_return_val_if_fail(event, GTKNTF_EVENT_PRIORITY_NORMAL);

	return event->priority;
}

void
gtkntf_event_set_show(GtkNtfEvent *event, gboolean show) {
	g_return_if_fail(event);

	event->show = show;
}

gboolean
gtkntf_event_get_show(GtkNtfEvent *event) {
	g_return_val_if_fail(event, FALSE);

	return event->show;
}

gboolean
gtkntf_event_show_notification(const gchar *n_type) {
	GtkNtfEvent *event;

	g_return_val_if_fail(n_type, FALSE);

	event = gtkntf_event_find_for_notification(n_type);
	if(event)
		return event->show;
		
	return FALSE;
}

const GList *gtkntf_events_get() {
	return events;
}

void
gtkntf_events_save() {
	GtkNtfEvent *event;
	GList *l = NULL, *e;

	for(e = events; e; e = e->next) {
		event = GTKNTF_EVENT(e->data);

		if(event->show)
			l = g_list_append(l, event->n_type);
	}

	oul_prefs_set_string_list(BEASY_PREFS_NTF_NOTIFICATIONS, l);
	g_list_free(l);
}

gint
gtkntf_events_count() {
	return g_list_length(events);
}

const gchar *
gtkntf_events_get_nth_name(gint nth) {
	GtkNtfEvent *event = g_list_nth_data(events, nth);

	return event->name;
}

const gchar *
gtkntf_events_get_nth_notification(gint nth) {
	GtkNtfEvent *event = g_list_nth_data(events, nth);

	return event->n_type;
}

/*******************************************************************************
 * Helpers
 ******************************************************************************/
/* first pass to see if a notification should be shown.. */
static gboolean
gtkntf_event_should_show(const gchar *notification) {
	if(gtkntf_display_screen_saver_is_running())
		return FALSE;
		
	if(!gtkntf_event_show_notification(notification))
		return FALSE;

	return TRUE;
}

static void
gtkntf_event_common(const gchar *n_type, const gchar *source, 
						const gchar *title, const gchar *content)
{
	GtkNtfNotification *notification = NULL;
	GtkNtfEventInfo *info = NULL;

	g_return_if_fail(n_type);

	if(!gtkntf_event_should_show(n_type))
		return;

	notification = gtkntf_notification_find_for_event(n_type);
	if(!notification)
		return;
	
	info = gtkntf_event_info_new(n_type);
	if(source)
		gtkntf_event_info_set_source(info, source);
	
	if(title)
		gtkntf_event_info_set_title(info, title);

	if(content)
		gtkntf_event_info_set_content(info, content);

	gtkntf_display_show_event(info, notification);
}


static void
gtkntf_event_info(gchar *source, gchar *title, gchar *content, const gpointer data)
{
	const gchar *notification = (const gchar *)data;

	gchar *plain_content = NULL;
	gchar *plain_title = NULL;
	
	plain_title   = oul_markup_strip_html(title);
	plain_content = oul_markup_strip_html(content);
	
	gtkntf_event_common(notification, source, plain_title, plain_content);
	
	g_free(plain_title);
	g_free(plain_content);
}

/*******************************************************************************
 * Subsystem
 ******************************************************************************/
void
gtkntf_events_init(void) {
	GList *l = NULL, *ll = NULL;

	gtkntf_event_new("info", TOKENS_DEFAULT "Rr", _("Information Notification"),
				 _("Displayed when received a information notification"),
				 GTKNTF_EVENT_PRIORITY_NORMAL);

	gtkntf_event_new("warn", TOKENS_DEFAULT "Rr", _("Warning Notification"),
				 _("Displayed when received a warning notification"),
				 GTKNTF_EVENT_PRIORITY_NORMAL);

	gtkntf_event_new("error", TOKENS_DEFAULT "Rr", _("Error Notification"),
				 _("Displayed when received a error notification"),
				 GTKNTF_EVENT_PRIORITY_NORMAL);

	gtkntf_event_new("fatal", TOKENS_DEFAULT "Rr", _("Fatal Notification"),
				 _("Displayed when received a fatal notification"),
				 GTKNTF_EVENT_PRIORITY_NORMAL);

	/* build notify events prefs */
	for(l = events; l; l = l->next) {
		GtkNtfEvent *event = GTKNTF_EVENT(l->data);

		ll = g_list_append(ll, event->n_type);
	}
	oul_prefs_add_string_list(BEASY_PREFS_NTF_NOTIFICATIONS, ll);
	g_list_free(ll);

	/* now that the pref is created, set the events that are supposed to be displayed
	 * to allow displaying  */
	l = oul_prefs_get_string_list(BEASY_PREFS_NTF_NOTIFICATIONS);
	for(ll = l;	ll; ll = ll->next) {
		gchar *event_name = (gchar *)ll->data;

		if(event_name) {
			GtkNtfEvent *event = gtkntf_event_find_for_notification(event_name);
			g_free(ll->data);

			if(event)
				event->show = TRUE;
		}
	}
	g_list_free(l);

	gpointer notify_handle = oul_notify_get_handle();
	gpointer gtkntf_handle = gtkntf_get_handle();
	
	oul_signal_connect(notify_handle, "notify-info", gtkntf_handle,
						OUL_CALLBACK(gtkntf_event_info), "info");

	
	oul_signal_connect(notify_handle, "notify-warn", gtkntf_handle,
							OUL_CALLBACK(gtkntf_event_info), "warn");

	oul_signal_connect(notify_handle, "notify-error", gtkntf_handle,
							OUL_CALLBACK(gtkntf_event_info), "error");

	
	oul_signal_connect(notify_handle, "notify-fatal", gtkntf_handle,
							OUL_CALLBACK(gtkntf_event_info), "fatal");
}

void
gtkntf_events_uninit() {
	GList *l, *ll;

	for(l = events; l; l = ll) {
		ll = l->next;
		gtkntf_event_destroy(GTKNTF_EVENT(l->data));
	}
}
