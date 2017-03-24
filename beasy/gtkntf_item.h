#ifndef GTKNTF_ITEM_H
#define GTKNTF_ITEM_H

#define GTKNTF_ITEM(obj) ((GtkNtfItem *)(obj))

typedef struct _GtkNtfItem GtkNtfItem;

typedef enum _GtkNtfItemType {
	GTKNTF_ITEM_TYPE_ICON = 0,
	GTKNTF_ITEM_TYPE_IMAGE,
	GTKNTF_ITEM_TYPE_TEXT,
	GTKNTF_ITEM_TYPE_UNKNOWN
} GtkNtfItemType;

typedef enum _GtkNtfItemPosition {
	GTKNTF_ITEM_POSITION_NW = 0,
	GTKNTF_ITEM_POSITION_N,
	GTKNTF_ITEM_POSITION_NE,
	GTKNTF_ITEM_POSITION_W,
	GTKNTF_ITEM_POSITION_C,
	GTKNTF_ITEM_POSITION_E,
	GTKNTF_ITEM_POSITION_SW,
	GTKNTF_ITEM_POSITION_S,
	GTKNTF_ITEM_POSITION_SE,
	GTKNTF_ITEM_POSITION_UNKNOWN
} GtkNtfItemPosition;

#include <gdk/gdk.h>

#include "gtkntf_event_info.h"
#include "gtkntf_item_icon.h"
#include "gtkntf_item_image.h"
#include "gtkntf_item_offset.h"
#include "gtkntf_item_text.h"
#include "gtkntf_notification.h"

#include "xmlnode.h"

G_BEGIN_DECLS

GtkNtfItem *	gtkntf_item_new(GtkNtfNotification *notification);
GtkNtfItem *	gtkntf_item_new_from_xmlnode(GtkNtfNotification *notification, xmlnode *node);
GtkNtfItem *	gtkntf_item_copy(GtkNtfItem *item);
xmlnode *		gtkntf_item_to_xmlnode(GtkNtfItem *item);
void 			gtkntf_item_destroy(GtkNtfItem *item);

void 			gtkntf_item_set_type(GtkNtfItem *item, GtkNtfItemType type);
GtkNtfItemType 	gtkntf_item_get_type(GtkNtfItem *item);
const gchar *	gtkntf_item_type_to_string(GtkNtfItemType type, gboolean i18n);
GtkNtfItemType 	gtkntf_item_type_from_string(const gchar *string, gboolean i18n);
const gchar *	gtkntf_item_position_to_string(GtkNtfItemPosition position, gboolean i18n);
GtkNtfItemPosition 	gtkntf_item_position_from_string(const gchar *position, gboolean i18n);
void 				gtkntf_item_set_notification(GtkNtfItem *item, GtkNtfNotification *notification);
GtkNtfNotification *gtkntf_item_get_notification(GtkNtfItem *item);
void 				gtkntf_item_set_horz_offset(GtkNtfItem *item, GtkNtfItemOffset *offset);
GtkNtfItemOffset *	gtkntf_item_get_horz_offset(GtkNtfItem *item);
void 				gtkntf_item_set_vert_offset(GtkNtfItem *item, GtkNtfItemOffset *offset);
GtkNtfItemOffset *	gtkntf_item_get_vert_offset(GtkNtfItem *item);
void 				gtkntf_item_set_position(GtkNtfItem *item, GtkNtfItemPosition position);
GtkNtfItemPosition 	gtkntf_item_get_position(GtkNtfItem *item);

void 				gtkntf_item_set_item_icon(GtkNtfItem *item, GtkNtfItemIcon *icon);
GtkNtfItemIcon *	gtkntf_item_get_item_icon(GtkNtfItem *item);
void 				gtkntf_item_set_item_image(GtkNtfItem *item, GtkNtfItemImage *image);
GtkNtfItemImage *	gtkntf_item_get_item_image(GtkNtfItem *item);
void 				gtkntf_item_set_item_text(GtkNtfItem *item, GtkNtfItemText *text);
GtkNtfItemText *	gtkntf_item_get_item_text(GtkNtfItem *item);

void 				gtkntf_item_get_render_position(gint *x, gint *y, gint width, gint height,
								 gint img_width, gint img_height, GtkNtfItem *item);

void 				gtkntf_item_render(GtkNtfItem *item, GdkPixbuf *pixbuf, GtkNtfEventInfo *info);

void 				gtkntf_items_swap(GtkNtfItem *item1, GtkNtfItem *item2);

G_END_DECLS

#endif /* GTKNTF_ITEM_H */
