#ifndef GTKNTF_ITEM_IMAGE_H
#define GTKNTF_ITEM_IMAGE_H

#define GTKNTF_ITEM_IMAGE(obj)	((GtkNtfItemImage *)(obj))

typedef struct _GtkNtfItemImage GtkNtfItemImage;

#include <glib.h>
#include <gdk/gdk.h>

#include "xmlnode.h"

#include "gtkntf_event_info.h"
#include "gtkntf_item.h"


G_BEGIN_DECLS

GtkNtfItemImage *	gtkntf_item_image_new(GtkNtfItem *item);
GtkNtfItemImage *	gtkntf_item_image_new_from_xmlnode(GtkNtfItem *item, xmlnode *node);
GtkNtfItemImage *	gtkntf_item_image_copy(GtkNtfItemImage *image);
xmlnode *			gtkntf_item_image_to_xmlnode(GtkNtfItemImage *image);
void 				gtkntf_item_image_destroy(GtkNtfItemImage *item_image);

void 				gtkntf_item_image_set_item(GtkNtfItemImage *item_image, GtkNtfItem *item);
GtkNtfItem *		gtkntf_item_image_get_item(GtkNtfItemImage *item_image);
void 				gtkntf_item_image_set_image(GtkNtfItemImage *item_image, const gchar *image);
const gchar *		gtkntf_item_image_get_image(GtkNtfItemImage *item_image);

void 				gtkntf_item_image_render(GtkNtfItemImage *item_image, GdkPixbuf *pixbuf, GtkNtfEventInfo *info);

G_END_DECLS

#endif /* GTKNTF_ITEM_IMAGE_H */
