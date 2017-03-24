#include <glib.h>
#include <gdk/gdk.h>

#include "internal.h"

#include "debug.h"
#include "xmlnode.h"


#include "gtkntf_event_info.h"
#include "gtkntf_gtk_utils.h"
#include "gtkntf_item.h"
#include "gtkntf_notification.h"
#include "gtkntf_theme.h"
#include "gtkntf_item_image.h"


struct _GtkNtfItemImage {

	GtkNtfItem *item;
	gchar *filename;
	
};



void
gtkntf_item_image_destroy(GtkNtfItemImage *item_image) {
	g_return_if_fail(item_image);

	item_image->item = NULL;

	if(item_image->filename) {
		g_free(item_image->filename);
		item_image->filename = NULL;
	}

	g_free(item_image);
	item_image = NULL;
}

GtkNtfItemImage *
gtkntf_item_image_new(GtkNtfItem *item) {
	GtkNtfItemImage *item_image;

	g_return_val_if_fail(item, NULL);

	item_image = g_new0(GtkNtfItemImage, 1);
	item_image->item = item;

	return item_image;
}

GtkNtfItemImage *
gtkntf_item_image_new_from_xmlnode(GtkNtfItem *item, xmlnode *node) {
	GtkNtfItemImage *item_image;

	g_return_val_if_fail(item, NULL);
	g_return_val_if_fail(node, NULL);

	item_image = gtkntf_item_image_new(item);

	item_image->filename = g_strdup(xmlnode_get_attrib(node, "filename"));
	if(!item_image) {
		oul_debug_info("GTKNotify",
						"** Error loading image item: 'Unknown filename'\n");
		gtkntf_item_image_destroy(item_image);
		return NULL;
	}

	return item_image;
}

GtkNtfItemImage *
gtkntf_item_image_copy(GtkNtfItemImage *image) {
	GtkNtfItemImage *new_image;

	g_return_val_if_fail(image, NULL);

	new_image = gtkntf_item_image_new(image->item);

	if(image->filename)
		new_image->filename = g_strdup(image->filename);

	return new_image;
}

xmlnode *
gtkntf_item_image_to_xmlnode(GtkNtfItemImage *image) {
	xmlnode *parent;

	parent = xmlnode_new("image");
	xmlnode_set_attrib(parent, "filename", image->filename);

	return parent;
}

void
gtkntf_item_image_set_item(GtkNtfItemImage *item_image, GtkNtfItem *item) {
	g_return_if_fail(item_image);
	g_return_if_fail(item);

	item_image->item = item;
}

GtkNtfItem *
gtkntf_item_image_get_item(GtkNtfItemImage *item_image) {
	g_return_val_if_fail(item_image, NULL);

	return item_image->item;
}

void
gtkntf_item_image_set_image(GtkNtfItemImage *item_image, const gchar *image) {
	g_return_if_fail(item_image);
	g_return_if_fail(image);

	item_image->filename = g_strdup(image);
}

const gchar *
gtkntf_item_image_get_image(GtkNtfItemImage *item_image) {
	g_return_val_if_fail(item_image, NULL);

	return item_image->filename;
}

void
gtkntf_item_image_render(GtkNtfItemImage *item_image, GdkPixbuf *pixbuf,
					 GtkNtfEventInfo *info)
{
	GtkNtfNotification *notification;
	GtkNtfTheme *theme;
	GdkPixbuf *image;
	gint x, y;
	gint width, height;
	gchar *filename;

	g_return_if_fail(item_image);
	g_return_if_fail(pixbuf);
	g_return_if_fail(info);

	notification = gtkntf_item_get_notification(item_image->item);
	theme = gtkntf_notification_get_theme(notification);
	filename = g_build_filename(gtkntf_theme_get_path(theme),
								item_image->filename, NULL);
	image = gdk_pixbuf_new_from_file(filename, NULL);
	g_free(filename);

	if(!image)
		return;

	width = gdk_pixbuf_get_width(image);
	height = gdk_pixbuf_get_height(image);

	gtkntf_item_get_render_position(&x, &y,
								width, height,
								gdk_pixbuf_get_width(pixbuf),
								gdk_pixbuf_get_height(pixbuf),
								item_image->item);

	gtkntf_gtk_pixbuf_clip_composite(image, x, y, pixbuf);

	g_object_unref(G_OBJECT(image));
}
