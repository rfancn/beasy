#include <glib.h>
#include <gdk/gdk.h>

#include "debug.h"

#include "internal.h"

#include "gtkntf_event_info.h"
#include "gtkntf_item.h"
#include "gtkntf_item_icon.h"
#include "gtkntf_item_image.h"
#include "gtkntf_item_offset.h"
#include "gtkntf_item_text.h"
#include "gtkntf_notification.h"
#include "gtkntf_theme.h"

struct _GtkNtfItem {
	GtkNtfNotification *notification;
	GtkNtfItemType type;
	GtkNtfItemPosition position;
	GtkNtfItemOffset *h_offset;
	GtkNtfItemOffset *v_offset;

	union {
		GtkNtfItemIcon *icon;
		GtkNtfItemText *text;
		GtkNtfItemImage *image;
	} u;
	
};


/* This setup sucks, but meh.. it works... be sure to update both the translated
 * and the English versions if you add or remove anything.  The english is used
 *  only for the files, so they're case insentive.  The i18n are used in the
 * theme editor.
 *
 * I'd fix this.. But whats translated doesn't match what is in the themes, and
 * while we could use the text that's used in the theme's as display text, why
 * mess with it while this, as ugly as it is, does work.
 */
const gchar *items_norm[] = { "icon", "image", "text", NULL };
const gchar *items_i18n[] = { N_("Icon"), N_("Image"), N_("Text"), NULL };

const gchar *positions_norm[] = { "north-west", "north", "north-east",
								  "west", "center", "east",
								  "south-west", "south", "south-east",
								  NULL };
const gchar *positions_i18n[] = { N_("North West"), N_("North"),
								  N_("North East"), N_("West"), N_("Center"),
								  N_("East"), N_("South West"), N_("South"),
								  N_("South East"), NULL };



void
gtkntf_item_get_render_position(gint *x, gint *y, gint width, gint height,
							gint img_width, gint img_height, GtkNtfItem *item)
{
	gint north, east, south, west, lat, lon;
	gint item_h_w, item_h_h;
	gint img_h_w, img_h_h;
	gint h_offset, v_offset;

	g_return_if_fail(item);

	item_h_w = width / 2;
	item_h_h = height / 2;
	img_h_w = img_width / 2;
	img_h_h = img_height / 2;

	if(item->h_offset) {
		if(gtkntf_item_offset_get_is_percentage(item->h_offset))
			h_offset = (img_width * gtkntf_item_offset_get_value(item->h_offset)) / 100;
		else
			h_offset = gtkntf_item_offset_get_value(item->h_offset);
	} else {
		h_offset = 0;
	}

	if(item->v_offset) {
		if(gtkntf_item_offset_get_is_percentage(item->v_offset))
			v_offset = (img_height * gtkntf_item_offset_get_value(item->v_offset)) / 100;
		else
			v_offset = gtkntf_item_offset_get_value(item->v_offset);
	} else {
		v_offset = 0;
	}

	north = v_offset;
	east = img_width - width + h_offset;
	south = img_height - height + v_offset;
	west = h_offset;
	lon = img_h_w - item_h_w + h_offset;
	lat = img_h_h - item_h_h + v_offset;

	switch(item->position) {
		case GTKNTF_ITEM_POSITION_NW:	*x = west;	*y = north;	break;
		case GTKNTF_ITEM_POSITION_N:	*x = lon;	*y = north;	break;
		case GTKNTF_ITEM_POSITION_NE:	*x = east;	*y = north;	break;
		case GTKNTF_ITEM_POSITION_W:	*x = west;	*y = lat;	break;
		case GTKNTF_ITEM_POSITION_C:	*x = lon;	*y = lat;	break;
		case GTKNTF_ITEM_POSITION_E:	*x = east;	*y = lat;	break;
		case GTKNTF_ITEM_POSITION_SW:	*x = west;	*y = south;	break;
		case GTKNTF_ITEM_POSITION_S:	*x = lon;	*y = south;	break;
		case GTKNTF_ITEM_POSITION_SE:	*x = east;	*y = south;	break;
		default:					*x = 0;		*y = 0;		break;
	}
}

const gchar *
gtkntf_item_type_to_string(GtkNtfItemType type, gboolean i18n) {
	g_return_val_if_fail(type < GTKNTF_ITEM_TYPE_UNKNOWN, NULL);
	g_return_val_if_fail(type >= 0, NULL);

	if(i18n)
		return _(items_i18n[type]);
	else
		return items_norm[type];
}

GtkNtfItemType
gtkntf_item_type_from_string(const gchar *string, gboolean i18n) {
	gint i;
	const gchar *val;

	g_return_val_if_fail(string, GTKNTF_ITEM_TYPE_UNKNOWN);

	for(i = 0; i < GTKNTF_ITEM_TYPE_UNKNOWN; i++) {
		if(i18n)
			val = _(items_i18n[i]);
		else
			val = items_norm[i];

		if(!val)
			return GTKNTF_ITEM_TYPE_UNKNOWN;

		if(!g_ascii_strcasecmp(string, val))
			return i;
	}

	return GTKNTF_ITEM_TYPE_UNKNOWN;
}

const gchar *
gtkntf_item_position_to_string(GtkNtfItemPosition position, gboolean i18n) {
	g_return_val_if_fail(position < GTKNTF_ITEM_POSITION_UNKNOWN, NULL);
	g_return_val_if_fail(position >= 0, NULL);

	if(i18n)
		return _(positions_i18n[position]);
	else
		return positions_norm[position];
}

GtkNtfItemPosition
gtkntf_item_position_from_string(const gchar *position, gboolean i18n) {
	gint i;
	const gchar *val;

	g_return_val_if_fail(position, GTKNTF_ITEM_POSITION_UNKNOWN);

	for(i = 0; i < GTKNTF_ITEM_POSITION_UNKNOWN; i++) {
		if(i18n)
			val = _(positions_i18n[i]);
		else
			val = positions_norm[i];

		if(!val)
			return GTKNTF_ITEM_POSITION_UNKNOWN;

		if(!g_ascii_strcasecmp(val, position))
			return i;
	}

	return GTKNTF_ITEM_POSITION_UNKNOWN;
}

GtkNtfItem *
gtkntf_item_new(GtkNtfNotification *notification) {
	GtkNtfItem *item;

	g_return_val_if_fail(notification, NULL);

	item = g_new0(GtkNtfItem, 1);
	item->notification = notification;

	return item;
}

GtkNtfItem *
gtkntf_item_new_from_xmlnode(GtkNtfNotification *notification, xmlnode *node) {
	GtkNtfItem *item;
	xmlnode *child;

	g_return_val_if_fail(node, NULL);
	g_return_val_if_fail(notification, NULL);

	item = gtkntf_item_new(notification);

	if(!item)
		return NULL;

	item->type = gtkntf_item_type_from_string(xmlnode_get_attrib(node, "type"), FALSE);
	if(item->type == GTKNTF_ITEM_TYPE_UNKNOWN) {
		oul_debug_info("GTKNotify", "** Error: unknown item type\n");
		gtkntf_item_destroy(item);
		return NULL;
	}

	if((child = xmlnode_get_child(node, "position"))) {
		item->position = gtkntf_item_position_from_string(xmlnode_get_attrib(child, "value"), FALSE);
		if(item->position == GTKNTF_ITEM_POSITION_UNKNOWN) {
			oul_debug_info("GTKNotify", "** Error: invalid position\n");
			gtkntf_item_destroy(item);
			return NULL;
		}
	} else { /* if we don't have a position we drop the item */
		oul_debug_info("GTKNotify", "** Error: no positioning found for item\n");
		gtkntf_item_destroy(item);
		return NULL;
	}

	/* if we don't have an offset node, we create it anyways so it can be
	 *  changed in the theme editor.
	 */
	if((child = xmlnode_get_child(node, "h_offset")))
		item->h_offset = gtkntf_item_offset_new_from_xmlnode(item, child);
	if(!item->h_offset)
		item->h_offset = gtkntf_item_offset_new(item);

	if((child = xmlnode_get_child(node, "v_offset")))
		item->v_offset = gtkntf_item_offset_new_from_xmlnode(item, child);
	if(!item->v_offset)
		item->v_offset = gtkntf_item_offset_new(item);

	switch(item->type) {
		case GTKNTF_ITEM_TYPE_ICON:
			if((child = xmlnode_get_child(node, "icon"))) {
				item->u.icon = gtkntf_item_icon_new_from_xmlnode(item, child);
				if(!item->u.icon) {
					gtkntf_item_destroy(item);
					return NULL;
				}
			} else {
				oul_debug_info("GTKNotify", "** Error loading icon item: 'No icon element found'\n");
				gtkntf_item_destroy(item);
				return NULL;
			}

			break;
		case GTKNTF_ITEM_TYPE_IMAGE:
			if((child = xmlnode_get_child(node, "image"))) {
				item->u.image = gtkntf_item_image_new_from_xmlnode(item, child);
				if(!item->u.image) {
					gtkntf_item_destroy(item);
					return NULL;
				}
			} else {
				oul_debug_info("GTKNotify", "** Error loading image item: 'No image element found'\n");
				gtkntf_item_destroy(item);
				return NULL;
			}

			break;
		case GTKNTF_ITEM_TYPE_TEXT:
			if((child = xmlnode_get_child(node, "text"))) {
				item->u.text = gtkntf_item_text_new_from_xmlnode(item, child);
				if(!item->u.text) {
					gtkntf_item_destroy(item);
					return NULL;
				}
			} else {
				oul_debug_info("GTKNotify", "** Error loading text item: 'No text element found'\n");
				gtkntf_item_destroy(item);
				return NULL;
			}

			break;
		case GTKNTF_ITEM_TYPE_UNKNOWN:
		default:
			oul_debug_info("GTKNotify", "** Error loading item: 'Unknown item type'\n");
			gtkntf_item_destroy(item);
			return NULL;
			break;
	}

	return item;
}

GtkNtfItem *
gtkntf_item_copy(GtkNtfItem *item) {
	GtkNtfItem *new_item;

	g_return_val_if_fail(item, NULL);

	new_item = gtkntf_item_new(item->notification);
	new_item->type = item->type;
	new_item->position = item->position;
	new_item->h_offset = gtkntf_item_offset_copy(item->h_offset);
	new_item->v_offset = gtkntf_item_offset_copy(item->v_offset);

	if(item->type == GTKNTF_ITEM_TYPE_ICON)
		new_item->u.icon = gtkntf_item_icon_copy(item->u.icon);
	else if(item->type == GTKNTF_ITEM_TYPE_ICON)
		new_item->u.image = gtkntf_item_image_copy(item->u.image);
	else if(item->type == GTKNTF_ITEM_TYPE_TEXT)
		new_item->u.text = gtkntf_item_text_copy(item->u.text);
	else {
		gtkntf_item_destroy(new_item);
		new_item = NULL;
	}

	return new_item;
}

xmlnode *
gtkntf_item_to_xmlnode(GtkNtfItem *item) {
	gchar *offset;
	xmlnode *parent, *child;

	parent = xmlnode_new("item");
	xmlnode_set_attrib(parent, "type", gtkntf_item_type_to_string(item->type, FALSE));

	child = xmlnode_new_child(parent, "position");
	xmlnode_set_attrib(child, "value", gtkntf_item_position_to_string(item->position, FALSE));

	child = xmlnode_new_child(parent, "h_offset");
	offset = g_strdup_printf("%d%s", gtkntf_item_offset_get_value(item->h_offset),
							 gtkntf_item_offset_get_is_percentage(item->h_offset) ? "%" : "");
	xmlnode_set_attrib(child, "value", offset);
	g_free(offset);

	child = xmlnode_new_child(parent, "v_offset");
	offset = g_strdup_printf("%d%s", gtkntf_item_offset_get_value(item->v_offset),
							 gtkntf_item_offset_get_is_percentage(item->v_offset) ? "%" : "");
	xmlnode_set_attrib(child, "value", offset);
	g_free(offset);

	switch(item->type) {
		case GTKNTF_ITEM_TYPE_ICON:
			child = gtkntf_item_icon_to_xmlnode(item->u.icon);
			break;
		case GTKNTF_ITEM_TYPE_IMAGE:
			child = gtkntf_item_image_to_xmlnode(item->u.image);
			break;
		case GTKNTF_ITEM_TYPE_TEXT:
			child = gtkntf_item_text_to_xmlnode(item->u.text);
			break;
		case GTKNTF_ITEM_TYPE_UNKNOWN:
		default:
			child = NULL;
	}

	if(child)
		xmlnode_insert_child(parent, child);

	return parent;
}

void
gtkntf_item_destroy(GtkNtfItem *item) {
	g_return_if_fail(item);

	if(item->h_offset) {
		gtkntf_item_offset_destroy(item->h_offset);
		item->h_offset = NULL;
	}

	if(item->v_offset) {
		gtkntf_item_offset_destroy(item->v_offset);
		item->v_offset = NULL;
	}

	if(item->type == GTKNTF_ITEM_TYPE_ICON && item->u.icon) {
		gtkntf_item_icon_destroy(item->u.icon);
		item->u.icon = NULL;
	}

	if(item->type == GTKNTF_ITEM_TYPE_IMAGE && item->u.image) {
		gtkntf_item_image_destroy(item->u.image);
		item->u.image = NULL;
	}

	if(item->type == GTKNTF_ITEM_TYPE_TEXT && item->u.text) {
		gtkntf_item_text_destroy(item->u.text);
		item->u.text = NULL;
	}

	g_free(item);
	item = NULL;
}

void
gtkntf_item_set_type(GtkNtfItem *item, GtkNtfItemType type) {
	g_return_if_fail(item);
	g_return_if_fail(type != GTKNTF_ITEM_TYPE_UNKNOWN);

	item->type = type;
}

GtkNtfItemType
gtkntf_item_get_type(GtkNtfItem *item) {
	g_return_val_if_fail(item, GTKNTF_ITEM_TYPE_UNKNOWN);

	return item->type;
}

void
gtkntf_item_set_notification(GtkNtfItem *item, GtkNtfNotification *notification) {
	g_return_if_fail(item);
	g_return_if_fail(notification);

	item->notification = notification;
}

GtkNtfNotification *
gtkntf_item_get_notification(GtkNtfItem *item) {
	g_return_val_if_fail(item, NULL);

	return item->notification;
}

void
gtkntf_item_set_horz_offset(GtkNtfItem *item, GtkNtfItemOffset *offset) {
	g_return_if_fail(item);
	g_return_if_fail(offset);

	item->h_offset = offset;
}

GtkNtfItemOffset *
gtkntf_item_get_horz_offset(GtkNtfItem *item) {
	g_return_val_if_fail(item, NULL);

	return item->h_offset;
}

void
gtkntf_item_set_vert_offset(GtkNtfItem *item, GtkNtfItemOffset *offset) {
	g_return_if_fail(item);
	g_return_if_fail(offset);

	item->v_offset = offset;
}

GtkNtfItemOffset *
gtkntf_item_get_vert_offset(GtkNtfItem *item) {
	g_return_val_if_fail(item, NULL);

	return item->v_offset;
}

void
gtkntf_item_set_position(GtkNtfItem *item, GtkNtfItemPosition position) {
	g_return_if_fail(item);
	g_return_if_fail(position != GTKNTF_ITEM_POSITION_UNKNOWN);

	item->position = position;
}

GtkNtfItemPosition
gtkntf_item_get_position(GtkNtfItem *item) {
	g_return_val_if_fail(item, GTKNTF_ITEM_POSITION_UNKNOWN);

	return item->position;
}

static void
gtkntf_item_free_old_subtype(GtkNtfItem *item) {
	if(item->type == GTKNTF_ITEM_TYPE_ICON && item->u.icon)
		gtkntf_item_icon_destroy(item->u.icon);
	else if(item->type == GTKNTF_ITEM_TYPE_IMAGE && item->u.image)
		gtkntf_item_image_destroy(item->u.image);
	else if(item->type == GTKNTF_ITEM_TYPE_TEXT && item->u.text)
		gtkntf_item_text_destroy(item->u.text);
}

void
gtkntf_item_set_item_icon(GtkNtfItem *item, GtkNtfItemIcon *icon) {
	g_return_if_fail(item);
	g_return_if_fail(icon);

	gtkntf_item_free_old_subtype(item);

	item->u.icon = icon;
}

GtkNtfItemIcon *
gtkntf_item_get_item_icon(GtkNtfItem *item) {
	g_return_val_if_fail(item->type == GTKNTF_ITEM_TYPE_ICON, NULL);

	return item->u.icon;
}

void
gtkntf_item_set_item_image(GtkNtfItem *item, GtkNtfItemImage *image) {
	g_return_if_fail(item);
	g_return_if_fail(image);

	gtkntf_item_free_old_subtype(item);

	item->u.image = image;
}

GtkNtfItemImage *
gtkntf_item_get_item_image(GtkNtfItem *item) {
	g_return_val_if_fail(item->type == GTKNTF_ITEM_TYPE_IMAGE, NULL);

	return item->u.image;
}

void
gtkntf_item_set_item_text(GtkNtfItem *item, GtkNtfItemText *text) {
	g_return_if_fail(item);
	g_return_if_fail(text);

	gtkntf_item_free_old_subtype(item);

	item->u.text = text;
}

GtkNtfItemText *
gtkntf_item_get_item_text(GtkNtfItem *item) {
	g_return_val_if_fail(item->type == GTKNTF_ITEM_TYPE_TEXT, NULL);

	return item->u.text;
}

void
gtkntf_item_render(GtkNtfItem *item, GdkPixbuf *pixbuf, GtkNtfEventInfo *info) {
	g_return_if_fail(item);
	g_return_if_fail(pixbuf);
	g_return_if_fail(info);

	switch(item->type) {
		case GTKNTF_ITEM_TYPE_ICON:
			gtkntf_item_icon_render(item->u.icon, pixbuf, info);
			break;
		case GTKNTF_ITEM_TYPE_TEXT:
			gtkntf_item_text_render(item->u.text, pixbuf, info);
			break;
		case GTKNTF_ITEM_TYPE_IMAGE:
			gtkntf_item_image_render(item->u.image, pixbuf, info);
			break;
		default:
			break;
	}
};

void
gtkntf_items_swap(GtkNtfItem *item1, GtkNtfItem *item2) {
	GtkNtfItem *item = NULL;
	GList *l = NULL, *l1 = NULL, *l2 = NULL;

	g_return_if_fail(item1);
	g_return_if_fail(item2);

	g_return_if_fail(item1->notification == item2->notification);

	for(l = gtkntf_notification_get_items(item1->notification); l; l = l->next) {
		if(l->data == item1)
			l1 = l;
		if(l->data == item2)
			l2 = l;
	}

	g_return_if_fail(l1);
	g_return_if_fail(l2);

	item = l1->data;
	l1->data = l2->data;
	l2->data = item;
}
