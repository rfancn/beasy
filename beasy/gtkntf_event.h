#ifndef GTKNTF_EVENT_H
#define GTKNTF_EVENT_H

/*
  * D: Date | d: day 01 -31 | H: hour 01-23 | h:hour 01-12 | M: month | m: minute | s: seconds
  * T:Time according to the theme var | t:seconds since the epoc
  */
#define TOKENS_DEFAULT "%YyMDdHhmsTt"

typedef enum _GtkNtfEventPriority {
	GTKNTF_EVENT_PRIORITY_LOWEST = -9999,
	GTKNTF_EVENT_PRIORITY_LOWER = -6666,
	GTKNTF_EVENT_PRIORITY_LOW = -3333,
	GTKNTF_EVENT_PRIORITY_NORMAL = 0,
	GTKNTF_EVENT_PRIORITY_HIGH = 3333,
	GTKNTF_EVENT_PRIORITY_HIGHER = 6666,
	GTKNTF_EVENT_PRIORITY_HIGHEST = 9999,
} GtkNtfEventPriority;

typedef struct _GtkNtfEvent GtkNtfEvent;

#define GTKNTF_EVENT(obj)	((GtkNtfEvent *)(obj))

G_BEGIN_DECLS

GtkNtfEvent *	gtkntf_event_new(const gchar *notification_type, const gchar *tokens,
					  const gchar *name, const gchar *description,
					  GtkNtfEventPriority priority);
GtkNtfEvent *	gtkntf_event_find_for_notification(const gchar *type);
void 			gtkntf_event_destroy(GtkNtfEvent *event);

const gchar *	gtkntf_event_get_notification_type(GtkNtfEvent *event);
const gchar *	gtkntf_event_get_tokens(GtkNtfEvent *event);
const gchar *	gtkntf_event_get_name(GtkNtfEvent *event);
const gchar *	gtkntf_event_get_description(GtkNtfEvent *event);
GtkNtfEventPriority gtkntf_event_get_priority(GtkNtfEvent *event);

void 			gtkntf_event_set_show(GtkNtfEvent *event, gboolean show);
gboolean 		gtkntf_event_get_show(GtkNtfEvent *event);
gboolean 		gtkntf_event_show_notification(const gchar *n_type);

const GList *	gtkntf_events_get();
void 			gtkntf_events_save();
gint 			gtkntf_events_count();
const gchar *	gtkntf_events_get_nth_name(gint nth);
const gchar *	gtkntf_events_get_nth_notification(gint nth);

void gtkntf_events_init();
void gtkntf_events_uninit();

G_END_DECLS

#endif /* GTKNTF_EVENT_H */
