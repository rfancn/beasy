#include <stdlib.h>
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
#include "gtkntf_theme_ops.h"
#include "gtkntf_utils.h"

struct _GtkNtfNotification {
	GtkNtfTheme *theme;
	gchar *n_type;
	gchar *alias;

	gboolean use_gtk;
	gchar *background;
	gint width;
	gint height;

	GList *items;
};

/*******************************************************************************
 * API
 ******************************************************************************/
GtkNtfNotification *
gtkntf_notification_new(GtkNtfTheme *theme) {
	GtkNtfNotification *notification;

	g_return_val_if_fail(theme, NULL);

	notification = g_new0(GtkNtfNotification, 1);
	notification->theme = theme;
	notification->use_gtk = TRUE;
	notification->height = 140;
	notification->width = 120;

	return notification;
}

GtkNtfNotification *
gtkntf_notification_new_from_xmlnode(GtkNtfTheme *theme, xmlnode *node) {
	GtkNtfNotification *notification;
	GtkNtfItem *item;
	xmlnode *child;
	const gchar *data;

	g_return_val_if_fail(theme, NULL);
	g_return_val_if_fail(node, NULL);

	notification = gtkntf_notification_new(theme);

	notification->n_type = g_strdup(xmlnode_get_attrib(node, "type"));
	if(!notification->n_type) {
		oul_debug_info("GTKNotify", "** Error: Notification type unknown\n");
		gtkntf_notification_destroy(notification);
		return NULL;
	}

	if(!g_utf8_collate(notification->n_type, GTKNTF_NOTIFICATION_MASTER))
		gtkntf_theme_set_master(theme, notification);

	data = xmlnode_get_attrib(node, "use_gtk");
	if(data)
		notification->use_gtk = atoi(data);

	data = xmlnode_get_attrib(node, "background");
	if(data)
		notification->background = g_strdup(data);

	data = xmlnode_get_attrib(node, "width");
	if(data)
		notification->width = atoi(data);

	data = xmlnode_get_attrib(node, "height");
	if(data)
		notification->height = atoi(data);

	data = xmlnode_get_attrib(node, "alias");
	if(data)
		notification->alias = g_strdup(data);

	if(notification->use_gtk) {
		if(notification->width < GTKNTF_NOTIFICATION_MIN ||
		   notification->height < GTKNTF_NOTIFICATION_MIN)
		{
			oul_debug_info("GTKNotify", "** Error: notification '%s' is using the "
							"gtk background but %dx%d is less than the %dx%d minimum\n",
							notification->n_type,
							notification->width, notification->height,
							GTKNTF_NOTIFICATION_MIN, GTKNTF_NOTIFICATION_MIN);
			gtkntf_notification_destroy(notification);
			return NULL;
		}
	} else if(!notification->background) {
		oul_debug_info("GTKNotify", "** Error: notification '%s' is not using the "
						"gtk background and does not have a background image\n",
						notification->n_type);
		gtkntf_notification_destroy(notification);
		return NULL;
	}

	child = xmlnode_get_child(node, "item");

	while(child) {
		item = gtkntf_item_new_from_xmlnode(notification, child);

		if(item)
			gtkntf_notification_add_item(notification, item);

		child = xmlnode_get_next_twin(child);
	}

	return notification;
}

GtkNtfNotification *
gtkntf_notification_copy(GtkNtfNotification *notification) {
	GtkNtfNotification *new_notification;
	GList *l;

	g_return_val_if_fail(notification, NULL);

	new_notification = gtkntf_notification_new(notification->theme);

	if(notification->n_type)
		new_notification->n_type = g_strdup(notification->n_type);

	if(notification->background)
		new_notification->background = g_strdup(notification->background);

	if(notification->alias)
		new_notification->alias = g_strdup(notification->alias);

	new_notification->use_gtk = notification->use_gtk;
	new_notification->width = notification->width;
	new_notification->height = notification->height;

	for(l = notification->items; l; l = l->next) {
		GtkNtfItem *item;

		item = gtkntf_item_copy(GTKNTF_ITEM(l->data));
		new_notification->items = g_list_append(new_notification->items, item);
	}

	return new_notification;
}

xmlnode *
gtkntf_notification_to_xmlnode(GtkNtfNotification *notification) {
	GList *l;
	xmlnode *parent, *child;
	gchar *data;

	parent = xmlnode_new("notification");
	xmlnode_set_attrib(parent, "type", notification->n_type);

	xmlnode_set_attrib(parent, "use_gtk", (notification->use_gtk) ? "1" : "0");

	if(notification->background)
		xmlnode_set_attrib(parent, "background", notification->background);

	if(notification->alias)
		xmlnode_set_attrib(parent, "alias", notification->alias);

	data = g_strdup_printf("%d", notification->width);
	xmlnode_set_attrib(parent, "width", data);
	g_free(data);

	data = g_strdup_printf("%d", notification->height);
	xmlnode_set_attrib(parent, "height", data);
	g_free(data);

	for(l = notification->items; l; l = l->next) {
		if((child = gtkntf_item_to_xmlnode(GTKNTF_ITEM(l->data))))
			xmlnode_insert_child(parent, child);
	}

	return parent;
}

void
gtkntf_notification_destroy(GtkNtfNotification *notification) {
	GtkNtfItem *item;
	GList *l;

	g_return_if_fail(notification);

	if(notification->n_type) {
		g_free(notification->n_type);
		notification->n_type = NULL;
	}

	if(notification->background) {
		g_free(notification->background);
		notification->background = NULL;
	}

	if(notification->alias) {
		g_free(notification->alias);
		notification->alias = NULL;
	}

	if(notification->items) {
		for(l = notification->items; l; l = l->next) {
			item = GTKNTF_ITEM(l->data);
			gtkntf_item_destroy(item);
		}

		g_list_free(notification->items);
		notification->items = NULL;
	}

	g_free(notification);
}

void
gtkntf_notification_set_type(GtkNtfNotification *notification, const gchar *n_type) {
	g_return_if_fail(notification);
	g_return_if_fail(n_type);

	if(notification->n_type)
		g_free(notification->n_type);

	notification->n_type = g_strdup(n_type);
}

const gchar *
gtkntf_notification_get_type(GtkNtfNotification *notification) {
	g_return_val_if_fail(notification, NULL);

	return notification->n_type;
}

void
gtkntf_notification_set_use_gtk(GtkNtfNotification *notification, gboolean value) {
	g_return_if_fail(notification);

	notification->use_gtk = value;
}

gboolean
gtkntf_notification_get_use_gtk(GtkNtfNotification *notification) {
	g_return_val_if_fail(notification, FALSE);

	return notification->use_gtk;
}

void
gtkntf_notification_set_background(GtkNtfNotification *notification,
							   const gchar *background)
{
	g_return_if_fail(notification);

	if(notification->background)
		g_free(notification->background);

	notification->background = g_strdup(background);
}

const gchar *
gtkntf_notification_get_background(GtkNtfNotification *notification) {
	g_return_val_if_fail(notification, NULL);

	return notification->background;
}

void
gtkntf_notification_set_width(GtkNtfNotification *notification, gint width) {
	g_return_if_fail(notification);

	notification->width = width;
}

gint
gtkntf_notification_get_width(GtkNtfNotification *notification) {
	g_return_val_if_fail(notification, -1);

	return notification->width;
}

void
gtkntf_notification_set_height(GtkNtfNotification *notification, gint height) {
	g_return_if_fail(notification);

	notification->height = height;
}

gint
gtkntf_notification_get_height(GtkNtfNotification *notification) {
	g_return_val_if_fail(notification, -1);

	return notification->height;
}

void
gtkntf_notification_add_item(GtkNtfNotification *notification, GtkNtfItem *item) {
	g_return_if_fail(notification);
	g_return_if_fail(item);

	notification->items = g_list_append(notification->items, item);
}

void
gtkntf_notification_remove_item(GtkNtfNotification *notification, GtkNtfItem *item) {
	g_return_if_fail(notification);
	g_return_if_fail(item);

	notification->items = g_list_remove(notification->items, item);
}

GList *
gtkntf_notification_get_items(GtkNtfNotification *notification) {
	g_return_val_if_fail(notification, NULL);

	return notification->items;
}

const gchar *
gtkntf_notification_get_alias(const GtkNtfNotification *notification) {
	g_return_val_if_fail(notification, NULL);

	return notification->alias;
}

void
gtkntf_notification_set_alias(GtkNtfNotification *notification, const gchar *alias) {
	g_return_if_fail(notification);

	if(notification->alias)
		g_free(notification->alias);

	notification->alias = (alias) ? g_strdup(alias) : NULL;
}

/*******************************************************************************
 * Finding, rendering, all that fun stuff...
 ******************************************************************************/
void
gtkntf_notifications_swap(GtkNtfNotification *notification1, GtkNtfNotification *notification2) {
	GtkNtfNotification *notification = NULL;
	GList *l = NULL, *l1 = NULL, *l2 = NULL;

	g_return_if_fail(notification1);
	g_return_if_fail(notification2);

	if(notification1->theme != notification2->theme)
		return;

	for(l = gtkntf_theme_get_notifications(notification1->theme); l; l = l->next) {
		if(l->data == notification1)
			l1 = l;
		if(l->data == notification2)
			l2 = l;
	}

	g_return_if_fail(l1);
	g_return_if_fail(l2);

	/* swap 'em */
	notification = l1->data;
	l1->data = l2->data;
	l2->data = notification;
}

GList *
gtkntf_notifications_for_event(const gchar *n_type) {
	GtkNtfTheme *theme;
	GtkNtfNotification *notification;
	GList *l = NULL, *t, *n;

	g_return_val_if_fail(n_type, NULL);

	for(t = gtkntf_themes_get_loaded(); t; t = t->next) {
		theme = GTKNTF_THEME(t->data);

		for(n = gtkntf_theme_get_notifications(theme); n; n = n->next) {
			notification = GTKNTF_NOTIFICATION(n->data);

			if(!g_ascii_strcasecmp(notification->n_type, n_type))
				l = g_list_append(l, notification);
		}
	}

	return l;
}

GtkNtfNotification *
gtkntf_notification_find_for_event(const gchar *n_type) {
	GtkNtfNotification *notification = NULL;
	GList *n = NULL;
	gint c;

	g_return_val_if_fail(n_type, NULL);

	n = gtkntf_notifications_for_event(n_type);
	if(!n)
		return NULL;

	c = rand() % g_list_length(n);

	notification = GTKNTF_NOTIFICATION(g_list_nth_data(n, c));
	g_list_free(n);

	return notification;
}

GtkNtfNotification *
gtkntf_notification_find_for_theme(GtkNtfTheme *theme, const gchar *n_type) {
	GtkNtfNotification *notification = NULL;
	GList *n = NULL, *t = NULL;
	gint len;

	g_return_val_if_fail(theme, NULL);
	g_return_val_if_fail(n_type, NULL);

	/* Get the list of notifications for a theme */
	for(t = gtkntf_theme_get_notifications(theme); t; t = t->next) {
		notification = GTKNTF_NOTIFICATION(t->data);

		if(!gtkntf_utils_strcmp(notification->n_type, n_type))
			n = g_list_append(n, notification);
	}

	len = g_list_length(n);

	if(len == 0)
		notification = NULL;
	else if(len == 1)
		notification = GTKNTF_NOTIFICATION(n->data);
	else {
		gint c;
		time_t t;

		t = time(NULL);
		srand(t);

		c = rand() % len;
		notification = GTKNTF_NOTIFICATION(g_list_nth_data(n, c));
	}

	g_list_free(n);

	return notification;
}

GdkPixbuf *
gtkntf_notification_render(GtkNtfNotification *notification, GtkNtfEventInfo *info) {
	GtkNtfItem *item = NULL;
	GdkPixbuf *pixbuf = NULL;
	GList *l = NULL;
	gchar *filename;
	const gchar *path;

	g_return_val_if_fail(notification, NULL);
	g_return_val_if_fail(info, NULL);

	if(notification->background) {
		/* create the pixbuf, return if it failed */
		path = gtkntf_theme_get_path(notification->theme);
		filename = g_build_filename(path, notification->background, NULL);
		pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
		g_free(filename);

		if(!pixbuf) {
			oul_debug_info("GTKNotify", "Couldn't not load notification background\n");
			return NULL;
		}
	} else {
		GdkPixmap *pixmap = NULL;

		pixmap = gtkntf_gtk_theme_get_bg_pixmap();

		if(pixmap) {
			GdkPixbuf *tile = NULL;
			gint width, height;

			gdk_drawable_get_size(GDK_DRAWABLE(pixmap), &width, &height);

			tile = gdk_pixbuf_get_from_drawable(NULL, GDK_DRAWABLE(pixmap), NULL,
												0, 0, 0, 0, width, height);

			if(!tile) {
				oul_debug_info("GTKNotify", "Failed to get the gtk theme "
								"background image\n");
				return NULL;
			}

			pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
									notification->width, notification->height);

			gtkntf_gtk_pixbuf_tile(pixbuf, tile);
			g_object_unref(G_OBJECT(tile));
		} else {
			GdkColor color;
			guint32 pixel;

			pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
									notification->width, notification->height);

			if(!pixbuf) {
				oul_debug_info("GTKNotify", "Failed to create notification background\n");
				return NULL;
			}

			gtkntf_gtk_theme_get_bg_color(&color);
			pixel = gtkntf_gtk_color_pixel_from_gdk(&color);
			gdk_pixbuf_fill(pixbuf, pixel);
		}
	}

	/* render the items */
	for(l = notification->items; l; l = l->next) {
		item = GTKNTF_ITEM(l->data);

		gtkntf_item_render(item, pixbuf, info);
	}

	/* display it already!! */
	return pixbuf;
}

GtkNtfTheme *
gtkntf_notification_get_theme(GtkNtfNotification *notification) {
	g_return_val_if_fail(notification, NULL);

	return notification->theme;
}
