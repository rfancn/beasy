#ifndef GTKNTF_NOTIFICATION_H
#define GTKNTF_NOTIFICATION_H

typedef struct _GtkNtfNotification GtkNtfNotification;

#define GTKNTF_NOTIFICATION(obj)	((GtkNtfNotification *)obj)

#define GTKNTF_NOTIFICATION_MASTER	"!master"
#define GTKNTF_NOTIFICATION_MIN		(16)
#define GTKNTF_NOTIFICATION_MAX		(512)

#include <glib.h>
#include <gdk/gdk.h>

#include "gtkntf_event_info.h"
#include "gtkntf_item.h"
#include "gtkntf_theme.h"

#include "xmlnode.h"


G_BEGIN_DECLS

GtkNtfNotification *	gtkntf_notification_new(GtkNtfTheme *theme);
GtkNtfNotification *	gtkntf_notification_new_from_xmlnode(GtkNtfTheme *theme, xmlnode *node);
GtkNtfNotification *	gtkntf_notification_copy(GtkNtfNotification *notification);
xmlnode *				gtkntf_notification_to_xmlnode(GtkNtfNotification *notification);
void 					gtkntf_notification_destroy(GtkNtfNotification *notification);

void 					gtkntf_notification_set_type(GtkNtfNotification *notification, const gchar *type);
const gchar *			gtkntf_notification_get_type(GtkNtfNotification *notification);
void 					gtkntf_notification_set_use_gtk(GtkNtfNotification *notification, gboolean value);
gboolean 				gtkntf_notification_get_use_gtk(GtkNtfNotification *notification);
void 					gtkntf_notification_set_background(GtkNtfNotification *notification, const gchar *background);
const gchar *			gtkntf_notification_get_background(GtkNtfNotification *notification);
void 					gtkntf_notification_set_width(GtkNtfNotification *notification, gint width);
gint 					gtkntf_notification_get_width(GtkNtfNotification *notification);
void 					gtkntf_notification_set_height(GtkNtfNotification *notification, gint height);
gint 					gtkntf_notification_get_height(GtkNtfNotification *notification);
void 					gtkntf_notification_set_alias(GtkNtfNotification *notification, const gchar *alias);
const gchar *			gtkntf_notification_get_alias(const GtkNtfNotification *notification);

void 					gtkntf_notification_add_item(GtkNtfNotification *notification, GtkNtfItem *item);
void 					gtkntf_notification_remove_item(GtkNtfNotification *notification, GtkNtfItem *item);
GList *					gtkntf_notification_get_items(GtkNtfNotification *notification);

void 					gtkntf_notifications_swap(GtkNtfNotification *notification1, GtkNtfNotification *notification2);
GList *					gtkntf_notifications_for_event(const gchar *type);
GtkNtfNotification *	gtkntf_notification_find_for_event(const gchar *type);
GtkNtfNotification *	gtkntf_notification_find_for_theme(GtkNtfTheme *theme, const gchar *n_type);
GdkPixbuf *				gtkntf_notification_render(GtkNtfNotification *notification, GtkNtfEventInfo *info);
GtkNtfTheme *			gtkntf_notification_get_theme(GtkNtfNotification *notification);

void 					gtkntf_notifications_init();
void 					gtkntf_notifications_uninit();

G_END_DECLS

#endif /* GTKNTF_NOTIFICATION_H */
