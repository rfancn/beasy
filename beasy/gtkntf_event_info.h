#ifndef GTKNTF_EVENT_INFO_H
#define GTKNTF_EVENT_INFO_H

typedef enum _GtkNtfEventType {
	GTKNTF_EVENT_TYPE_UNKNOWN = 0,
	GTKNTF_EVENT_TYPE_BUDDY,
	GTKNTF_EVENT_TYPE_CONVERSATION,
	GTKNTF_EVENT_TYPE_ACCOUNT,
	GTKNTF_EVENT_TYPE_MISC
} GtkNtfEventType;

typedef struct _GtkNtfEventInfo GtkNtfEventInfo;

#include <glib.h>

#include "gtkntf_event.h"
#include "gtkntf_notification.h"


G_BEGIN_DECLS

GtkNtfEventInfo *gtkntf_event_info_new(const gchar *notification_type);
void 			gtkntf_event_info_destroy(GtkNtfEventInfo *info);

GtkNtfEvent *	gtkntf_event_info_get_event(GtkNtfEventInfo *info);

void			gtkntf_event_info_set_source(GtkNtfEventInfo *info, const gchar *source);
void 			gtkntf_event_info_set_content(GtkNtfEventInfo *info, const gchar *content);
const gchar *	gtkntf_event_info_get_content(GtkNtfEventInfo *info);
void 			gtkntf_event_info_set_timeout_id(GtkNtfEventInfo *info, guint timeout_id);
guint 			gtkntf_event_info_get_timeout_id(GtkNtfEventInfo *info);
void 			gtkntf_event_info_set_is_contact(GtkNtfEventInfo *info, gboolean value);
gboolean 		gtkntf_event_info_get_is_contact(GtkNtfEventInfo *info);
void 			gtkntf_event_info_set_open_action(GtkNtfEventInfo *info, GCallback open_action);
GCallback 		gtkntf_event_info_get_open_action(const GtkNtfEventInfo *info);
const gchar *	gtkntf_event_info_get_title(GtkNtfEventInfo *info);
const gchar *	gtkntf_event_info_get_source(GtkNtfEventInfo *info);


G_END_DECLS

#endif /* GTKNTF_EVENT_INFO_H */
