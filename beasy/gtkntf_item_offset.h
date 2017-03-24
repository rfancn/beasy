#ifndef GTKNTF_ITEM_OFFSET_H
#define GTKNTF_ITEM_OFFSET_H

typedef struct _GtkNtfItemOffset GtkNtfItemOffset;

#include "xmlnode.h"
#include "gtkntf_item.h"

G_BEGIN_DECLS

GtkNtfItemOffset *	gtkntf_item_offset_new(GtkNtfItem *item);
GtkNtfItemOffset *	gtkntf_item_offset_new_from_xmlnode(GtkNtfItem *item, xmlnode *node);
GtkNtfItemOffset *	gtkntf_item_offset_copy(GtkNtfItemOffset *offset);
void 				gtkntf_item_offset_destroy(GtkNtfItemOffset *offset);

void 				gtkntf_item_offset_set_value(GtkNtfItemOffset *offset, gint value);
gint 				gtkntf_item_offset_get_value(GtkNtfItemOffset *offset);

void 				gtkntf_item_offset_set_is_percentage(GtkNtfItemOffset *offset, gboolean is_offset);
gboolean 			gtkntf_item_offset_get_is_percentage(GtkNtfItemOffset *offset);

GtkNtfItem *		gtkntf_item_offset_get_item(GtkNtfItemOffset *offset);

G_END_DECLS

#endif /* GTKNTF_ITEM_OFFSET_H */
