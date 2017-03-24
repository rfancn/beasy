#ifndef GTKNTF_ITEM_ICON_H
#define GTKNTF_ITEM_ICON_H

typedef enum _GtkNtfItemIconType {
	GTKNTF_ITEM_ICON_TYPE_SYSTEM = 0,
	GTKNTF_ITEM_ICON_TYPE_FILE,
	GTKNTF_ITEM_ICON_TYPE_UNKNOWN
} GtkNtfItemIconType;

typedef enum _GtkNtfItemIconSize {
	GTKNTF_ITEM_ICON_SIZE_TINY = 0,
	GTKNTF_ITEM_ICON_SIZE_SMALL,
	GTKNTF_ITEM_ICON_SIZE_LITTLE,
	GTKNTF_ITEM_ICON_SIZE_NORMAL,
	GTKNTF_ITEM_ICON_SIZE_BIG,
	GTKNTF_ITEM_ICON_SIZE_LARGE,
	GTKNTF_ITEM_ICON_SIZE_HUGE,
	GTKNTF_ITEM_ICON_SIZE_UNKNOWN
} GtkNtfItemIconSize;

#define GTKNTF_ITEM_ICON(obj)	((GtkNtfItemIcon *)(obj))

typedef struct _GtkNtfItemIcon GtkNtfItemIcon;

#include <gdk/gdk.h>

#include "xmlnode.h"
#include "gtkntf_event_info.h"
#include "gtkntf_item.h"


G_BEGIN_DECLS

GtkNtfItemIcon *	gtkntf_item_icon_new(GtkNtfItem *item);
GtkNtfItemIcon *	gtkntf_item_icon_new_from_xmlnode(GtkNtfItem *item, xmlnode *node);
GtkNtfItemIcon *	gtkntf_item_icon_copy(GtkNtfItemIcon *icon);
xmlnode *			gtkntf_item_icon_to_xmlnode(GtkNtfItemIcon *icon);
void 				gtkntf_item_icon_destroy(GtkNtfItemIcon *item_icon);

void 				gtkntf_item_icon_set_type(GtkNtfItemIcon *icon, GtkNtfItemIconType type);
GtkNtfItemIconType 	gtkntf_item_icon_get_type(GtkNtfItemIcon *icon);
void 				gtkntf_item_icon_set_size(GtkNtfItemIcon *icon, GtkNtfItemIconSize size);
GtkNtfItemIconSize 	gtkntf_item_icon_get_size(GtkNtfItemIcon *icon);

void 				gtkntf_item_icon_set_item(GtkNtfItemIcon *icon, GtkNtfItem *item);
GtkNtfItem *		gtkntf_item_icon_get_item(GtkNtfItemIcon *icon);

void 				gtkntf_item_icon_render(GtkNtfItemIcon *item_icon, GdkPixbuf *pixbuf,
						 						GtkNtfEventInfo *info);

G_END_DECLS

#endif /* GTKNTF_ITEM_ICON_H */
