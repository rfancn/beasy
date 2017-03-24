#ifndef GTKNTF_DISPLAY_H
#define GTKNTF_DISPLAY_H

typedef struct _GtkNtfDisplay GtkNtfDisplay;

#define GTKNTF_DISPLAY(obj)	((GtkNtfDisplay *)obj)

typedef enum {
	GTKNTF_DISPLAY_STATE_UNKNOWN = 0,
	GTKNTF_DISPLAY_STATE_SHOWING,
	GTKNTF_DISPLAY_STATE_SHOWN,
	GTKNTF_DISPLAY_STATE_HIDING,
	GTKNTF_DISPLAY_STATE_DESTROYED
} GtkNtfDisplayState;

typedef enum _GtkNtfDisplayPosition {
	GTKNTF_DISPLAY_POSITION_NW = 0,
	GTKNTF_DISPLAY_POSITION_NE,
	GTKNTF_DISPLAY_POSITION_SW,
	GTKNTF_DISPLAY_POSITION_SE,
	GTKNTF_DISPLAY_POSITION_UNKNOWN
} GtkNtfDisplayPosition;

typedef enum _GtkNtfDisplayZoom {
	GTKNTF_DISPLAY_ZOOM_200 = 0,
	GTKNTF_DISPLAY_ZOOM_175,
	GTKNTF_DISPLAY_ZOOM_150,
	GTKNTF_DISPLAY_ZOOM_125,
	GTKNTF_DISPLAY_ZOOM_100,
	GTKNTF_DISPLAY_ZOOM_75,
	GTKNTF_DISPLAY_ZOOM_50,
	GTKNTF_DISPLAY_ZOOM_25,
	GTKNTF_DISPLAY_ZOOM_UNKNOWN
} GtkNtfDisplayZoom;

#include <glib.h>

#include "gtkntf_event_info.h"
#include "gtkntf_notification.h"


G_BEGIN_DECLS

GtkNtfDisplay *		gtkntf_display_new();
void 				gtkntf_display_destroy(GtkNtfDisplay *display);

GtkNtfEventInfo *	gtkntf_display_get_event_info(GtkNtfDisplay *display);

void 				gtkntf_display_show_event(GtkNtfEventInfo *info, GtkNtfNotification *notification);

void 				gtkntf_display_init();
void 				gtkntf_display_uninit();

#if GTK_CHECK_VERSION(2,2,0)
gint gtkntf_display_get_default_screen();
gint gtkntf_display_get_screen_count();
gint gtkntf_display_get_default_monitor();
gint gtkntf_display_get_monitor_count();
#endif /* GTK_CHECK_VERSION(2,2,0) */

gboolean gtkntf_display_screen_saver_is_running();

G_END_DECLS

#endif /* GTKNTF_DISPLAY_H */
