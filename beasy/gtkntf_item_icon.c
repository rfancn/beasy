#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "internal.h"

#include "debug.h"
#include "xmlnode.h"

#include "gtkntf_item.h"
#include "gtkntf_gtk_utils.h"

#include "gtkntf_theme.h"
#include "gtkntf_notification.h"
#include "gtkntf_item_icon.h"

struct _GtkNtfItemIcon {
	GtkNtfItem *item;

	GtkNtfItemIconType type;
	GtkNtfItemIconSize size;

	gchar *path;
	gchar *filename;
};


static GtkNtfItemIconType
item_icon_type_from_string(const gchar *string) {
	g_return_val_if_fail(string, GTKNTF_ITEM_ICON_TYPE_UNKNOWN);

	if(!g_ascii_strcasecmp(string, "system"))
		return GTKNTF_ITEM_ICON_TYPE_SYSTEM;
	else if(!g_ascii_strcasecmp(string, "file"))
		return GTKNTF_ITEM_ICON_TYPE_FILE;
	else
		return GTKNTF_ITEM_ICON_TYPE_UNKNOWN;
}

static const gchar *
item_icon_type_to_string(GtkNtfItemIconType type) {
	g_return_val_if_fail(type != GTKNTF_ITEM_ICON_TYPE_UNKNOWN, NULL);

	switch(type) {
		case GTKNTF_ITEM_ICON_TYPE_SYSTEM:		return "system";	break;
		case GTKNTF_ITEM_ICON_TYPE_FILE:		return "file";		break;
		case GTKNTF_ITEM_ICON_TYPE_UNKNOWN:
		default:								return NULL;		break;
	}
}

static GtkNtfItemIconSize
item_icon_size_from_string(const gchar *string) {
	g_return_val_if_fail(string, GTKNTF_ITEM_ICON_SIZE_UNKNOWN);

	if(!g_ascii_strcasecmp(string, "tiny"))
		return GTKNTF_ITEM_ICON_SIZE_TINY;
	else if(!g_ascii_strcasecmp(string, "small"))
		return GTKNTF_ITEM_ICON_SIZE_SMALL;
	else if(!g_ascii_strcasecmp(string, "little"))
		return GTKNTF_ITEM_ICON_SIZE_LITTLE;
	else if(!g_ascii_strcasecmp(string, "normal"))
		return GTKNTF_ITEM_ICON_SIZE_NORMAL;
	else if(!g_ascii_strcasecmp(string, "big"))
		return GTKNTF_ITEM_ICON_SIZE_BIG;
	else if(!g_ascii_strcasecmp(string, "large"))
		return GTKNTF_ITEM_ICON_SIZE_LARGE;
	else if(!g_ascii_strcasecmp(string, "huge"))
		return GTKNTF_ITEM_ICON_SIZE_HUGE;
	else
		return GTKNTF_ITEM_ICON_SIZE_UNKNOWN;
}

static const gchar *
item_icon_size_to_string(GtkNtfItemIconSize size) {
	g_return_val_if_fail(size != GTKNTF_ITEM_ICON_SIZE_UNKNOWN, NULL);

	switch(size) {
		case GTKNTF_ITEM_ICON_SIZE_HUGE:	return "huge";	break;
		case GTKNTF_ITEM_ICON_SIZE_LARGE:	return "large";	break;
		case GTKNTF_ITEM_ICON_SIZE_BIG:		return "big";	break;
		case GTKNTF_ITEM_ICON_SIZE_NORMAL:	return "normal";	break;
		case GTKNTF_ITEM_ICON_SIZE_LITTLE:	return "little";	break;
		case GTKNTF_ITEM_ICON_SIZE_SMALL:	return "small";	break;
		case GTKNTF_ITEM_ICON_SIZE_TINY:	return "tiny";	break;
		case GTKNTF_ITEM_ICON_SIZE_UNKNOWN:
		default:						return NULL;	break;
	}
}

void
gtkntf_item_icon_destroy(GtkNtfItemIcon *item_icon) {
	g_return_if_fail(item_icon);

	item_icon->item = NULL;
	item_icon->type = GTKNTF_ITEM_ICON_TYPE_UNKNOWN;
	item_icon->size = GTKNTF_ITEM_ICON_SIZE_UNKNOWN;

	if(item_icon->path){
		g_free(item_icon->path);
		item_icon->path = NULL;
	}
	
	if(item_icon->filename){
		g_free(item_icon->filename);
		item_icon->filename = NULL;
	}

	g_free(item_icon);
	item_icon = NULL;
}

GtkNtfItemIcon *
gtkntf_item_icon_new(GtkNtfItem *item) {
	GtkNtfItemIcon *item_icon;

	g_return_val_if_fail(item, NULL);

	item_icon = g_new0(GtkNtfItemIcon, 1);
	item_icon->item = item;

	return item_icon;
}

GtkNtfItemIcon *
gtkntf_item_icon_new_from_xmlnode(GtkNtfItem *item, xmlnode *node) {
	GtkNtfItemIcon *item_icon;

	g_return_val_if_fail(item, NULL);
	g_return_val_if_fail(node, NULL);

	item_icon = gtkntf_item_icon_new(item);

	item_icon->type = item_icon_type_from_string(xmlnode_get_attrib(node, "type"));
	if(item_icon->type == GTKNTF_ITEM_ICON_TYPE_UNKNOWN) {
		oul_debug_info("GTKNotify", "** Error loading icon item: 'Unknown icon type'\n");
		gtkntf_item_icon_destroy(item_icon);
		return NULL;
	}

	if(item_icon->type == GTKNTF_ITEM_ICON_TYPE_FILE){
		GtkNtfNotification *notification = gtkntf_item_get_notification(item);
		GtkNtfTheme *theme 	= gtkntf_notification_get_theme(notification);
		item_icon->filename = g_strdup_printf("%s.png", gtkntf_notification_get_type(notification));
		item_icon->path		= g_strdup(gtkntf_theme_get_path(theme));
	}

	item_icon->size = item_icon_size_from_string(xmlnode_get_attrib(node, "size"));
	if(item_icon->size == GTKNTF_ITEM_ICON_SIZE_UNKNOWN) {
		oul_debug_info("GTKNotify", "** Error loading icon item: 'Unknown icon size'\n");
		gtkntf_item_icon_destroy(item_icon);
		return NULL;
	}

	return item_icon;
}

GtkNtfItemIcon *
gtkntf_item_icon_copy(GtkNtfItemIcon *icon) {
	GtkNtfItemIcon *new_icon;

	g_return_val_if_fail(icon, NULL);

	new_icon = gtkntf_item_icon_new(icon->item);

	new_icon->type = icon->type;
	new_icon->size = icon->size;

	return new_icon;
}

xmlnode *
gtkntf_item_icon_to_xmlnode(GtkNtfItemIcon *icon) {
	xmlnode *parent;

	parent = xmlnode_new("icon");
	xmlnode_set_attrib(parent, "type", item_icon_type_to_string(icon->type));
	xmlnode_set_attrib(parent, "size", item_icon_size_to_string(icon->size));

	return parent;
}

void
gtkntf_item_icon_set_type(GtkNtfItemIcon *icon, GtkNtfItemIconType type) {
	g_return_if_fail(icon);
	g_return_if_fail(type != GTKNTF_ITEM_ICON_TYPE_UNKNOWN);

	icon->type = type;
}

GtkNtfItemIconType
gtkntf_item_icon_get_type(GtkNtfItemIcon *icon) {
	g_return_val_if_fail(icon, GTKNTF_ITEM_ICON_TYPE_UNKNOWN);

	return icon->type;
}

void
gtkntf_item_icon_set_size(GtkNtfItemIcon *icon, GtkNtfItemIconSize size) {
	g_return_if_fail(icon);
	g_return_if_fail(size != GTKNTF_ITEM_ICON_SIZE_UNKNOWN);

	icon->size = size;
}

GtkNtfItemIconSize
gtkntf_item_icon_get_size(GtkNtfItemIcon *icon) {
	g_return_val_if_fail(icon, GTKNTF_ITEM_ICON_SIZE_UNKNOWN);

	return icon->size;
}

void
gtkntf_item_icon_set_item(GtkNtfItemIcon *icon, GtkNtfItem *item) {
	g_return_if_fail(icon);
	g_return_if_fail(item);

	icon->item = item;
}

GtkNtfItem *
gtkntf_item_icon_get_item(GtkNtfItemIcon *icon) {
	g_return_val_if_fail(icon, NULL);

	return icon->item;
}

static void
get_icon_dimensions(gint *width, gint *height, GtkNtfItemIconSize size) {
	switch(size) {
		case GTKNTF_ITEM_ICON_SIZE_TINY:
			*width = *height = 16; break;
		case GTKNTF_ITEM_ICON_SIZE_SMALL:
			*width = *height = 24; break;
		case GTKNTF_ITEM_ICON_SIZE_LITTLE:
			*width = *height = 32; break;
		case GTKNTF_ITEM_ICON_SIZE_BIG:
			*width = *height = 64; break;
		case GTKNTF_ITEM_ICON_SIZE_LARGE:
			*width = *height = 96; break;
		case GTKNTF_ITEM_ICON_SIZE_HUGE:
			*width = *height = 144; break;
		case GTKNTF_ITEM_ICON_SIZE_NORMAL:
		default:
			*width = *height = 48; break;
	}
}

static void
get_icon_position(gint *x, gint *y,
				  gint img_width, gint img_height,
				  GtkNtfItemIcon *item_icon)
{
	gint width = 0, height = 0;

	g_return_if_fail(item_icon);

	get_icon_dimensions(&width, &height, item_icon->size);

	gtkntf_item_get_render_position(x, y, width, height, img_width, img_height,
								item_icon->item);
}

void
gtkntf_item_icon_render(GtkNtfItemIcon *item_icon, GdkPixbuf *pixbuf, GtkNtfEventInfo *info)
{
	GtkNtfEvent *event;
	GdkPixbuf *original = NULL, *scaled = NULL;
	gint x, y;
	gint width, height;
	gboolean is_contact;

	g_return_if_fail(item_icon);
	g_return_if_fail(pixbuf);
	g_return_if_fail(info);

	event = gtkntf_event_info_get_event(info);

	switch(item_icon->type){
		case GTKNTF_ITEM_ICON_TYPE_FILE:
			if(item_icon->path && item_icon->filename){
				gchar *iconfile = g_build_filename(item_icon->path, item_icon->filename, NULL);
				
				original = gdk_pixbuf_new_from_file(iconfile, NULL);
				g_free(iconfile);
			}
			break;
		case GTKNTF_ITEM_ICON_TYPE_SYSTEM:
		default:
			break;
	}
	
	/* if the original doesn't work for whatever reason we fallback to the
	  * protocol or the Pidgin guy(?) if it is a contact notification, this will
	  * be optional when we "enhance" the theme format.  If it fails we return
	  * like we used to.
	  */
	if(!original)
		return;
	
	get_icon_position(&x, &y,
					  gdk_pixbuf_get_width(pixbuf),
					  gdk_pixbuf_get_height(pixbuf),
					  item_icon);

	get_icon_dimensions(&width, &height, item_icon->size);
	scaled = gdk_pixbuf_scale_simple(original, width, height, GDK_INTERP_BILINEAR);
	g_object_unref(G_OBJECT(original));

	gtkntf_gtk_pixbuf_clip_composite(scaled, x, y, pixbuf);

	g_object_unref(G_OBJECT(scaled));
}
