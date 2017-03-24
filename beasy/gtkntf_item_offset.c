#include <glib.h>
#include <string.h>
#include <stdlib.h>

#include "internal.h"
#include "xmlnode.h"

#include "gtkntf_item.h"
#include "gtkntf_item_offset.h"

struct _GtkNtfItemOffset {
	GtkNtfItem *item;
	gint value;
	gboolean percentage;
}; 


void
gtkntf_item_offset_destroy(GtkNtfItemOffset *item_offset) {
	g_return_if_fail(item_offset);

	item_offset->item = NULL;
	item_offset->value = 0;
	item_offset->percentage = FALSE;

	g_free(item_offset);
	item_offset = NULL;
}

GtkNtfItemOffset *
gtkntf_item_offset_new(GtkNtfItem *item) {
	GtkNtfItemOffset *offset;

	g_return_val_if_fail(item, NULL);

	offset = g_new0(GtkNtfItemOffset, 1);
	offset->item = item;

	return offset;
}

GtkNtfItemOffset *
gtkntf_item_offset_new_from_xmlnode(GtkNtfItem *item, xmlnode *node) {
	GtkNtfItemOffset *offset;
	const gchar *data;

	g_return_val_if_fail(item, NULL);
	g_return_val_if_fail(node, NULL);

	offset = gtkntf_item_offset_new(item);

	data = xmlnode_get_attrib(node, "value");
	if(!data) {
		gtkntf_item_offset_destroy(offset);
		return NULL;
	}

	if(data[strlen(data) - 1] == '%')
		offset->percentage = TRUE;

	offset->value = atoi(data);

	return offset;
}

GtkNtfItemOffset *
gtkntf_item_offset_copy(GtkNtfItemOffset *offset) {
	GtkNtfItemOffset *new_offset;

	g_return_val_if_fail(offset, NULL);

	new_offset = gtkntf_item_offset_new(offset->item);
	new_offset->value = offset->value;
	new_offset->percentage = offset->percentage;

	return new_offset;
}

void
gtkntf_item_offset_set_value(GtkNtfItemOffset *offset, gint value) {
	g_return_if_fail(offset);

	offset->value = value;
}

gint
gtkntf_item_offset_get_value(GtkNtfItemOffset *offset) {
	g_return_val_if_fail(offset, -1);

	return offset->value;
}

void
gtkntf_item_offset_set_is_percentage(GtkNtfItemOffset *offset, gboolean is_percentage) {
	g_return_if_fail(offset);

	offset->percentage = is_percentage;
}

gboolean
gtkntf_item_offset_get_is_percentage(GtkNtfItemOffset *offset) {
	g_return_val_if_fail(offset, FALSE);

	return offset->percentage;
}

GtkNtfItem *
gtkntf_item_offset_get_item(GtkNtfItemOffset *offset) {
	g_return_val_if_fail(offset, NULL);

	return offset->item;
}
