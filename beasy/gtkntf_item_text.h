#ifndef GTKNTF_ITEM_TEXT_H
#define GTKNTF_ITEM_TEXT_H

typedef struct _GtkNtfItemText GtkNtfItemText;

#define GTKNTF_ITEM_TEXT(obj)	((GtkNtfItemText *)(obj))

typedef enum _GtkNtfItemTextClipping {
	GTKNTF_ITEM_TEXT_CLIPPING_TRUNCATE = 0,
	GTKNTF_ITEM_TEXT_CLIPPING_ELLIPSIS_START,
	GTKNTF_ITEM_TEXT_CLIPPING_ELLIPSIS_MIDDLE,
	GTKNTF_ITEM_TEXT_CLIPPING_ELLIPSIS_END,
	GTKNTF_ITEM_TEXT_CLIPPING_UNKNOWN
} GtkNtfItemTextClipping;

#include <glib.h>
#include <gdk/gdk.h>

#include "xmlnode.h"
#include "gtkntf_item.h"
#include "gtkntf_event_info.h"

G_BEGIN_DECLS

void gtkntf_item_text_init();
void gtkntf_item_text_uninit();

GtkNtfItemText *gtkntf_item_text_new(GtkNtfItem *item);
GtkNtfItemText *gtkntf_item_text_new_from_xmlnode(GtkNtfItem *item, xmlnode *node);
GtkNtfItemText *gtkntf_item_text_copy(GtkNtfItemText *text);
xmlnode *gtkntf_item_text_to_xmlnode(GtkNtfItemText *text);
void gtkntf_item_text_destroy(GtkNtfItemText *item_text);

void gtkntf_item_text_set_format(GtkNtfItemText *item_text, const gchar *format);
const gchar *gtkntf_item_text_get_format(GtkNtfItemText *item_text);
void gtkntf_item_text_set_font(GtkNtfItemText *item_text, const gchar *font);
const gchar *gtkntf_item_text_get_font(GtkNtfItemText *item_text);
void gtkntf_item_text_set_color(GtkNtfItemText *item_text, const gchar *color);
const gchar *gtkntf_item_text_get_color(GtkNtfItemText *item_text);
void gtkntf_item_text_set_clipping(GtkNtfItemText *item_text, GtkNtfItemTextClipping clipping);
GtkNtfItemTextClipping gtkntf_item_text_get_clipping(GtkNtfItemText *item_text);
void gtkntf_item_text_set_width(GtkNtfItemText *item_text, gint width);
gint gtkntf_item_text_get_width(GtkNtfItemText *item_text);
void gtkntf_item_text_set_item(GtkNtfItemText *item_text, GtkNtfItem *item);
GtkNtfItem *gtkntf_item_text_get_item(GtkNtfItemText *item_text);

void gtkntf_item_text_render(GtkNtfItemText *item_text, GdkPixbuf *pixbuf, GtkNtfEventInfo *info);

G_END_DECLS

#endif /* GTKNTF_ITEM_TEXT_H */
