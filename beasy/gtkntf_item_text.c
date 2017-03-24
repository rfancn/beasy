#include <ft2build.h>
#include <freetype/ftimage.h>
#include <pango/pangoft2.h>

#include <stdio.h>
#include <stdlib.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <pango/pango.h>
#include <string.h>
#include <time.h>

#include "internal.h"

#include "debug.h"
#include "network.h"
#include "status.h"
#include "util.h"
#include "xmlnode.h"
#include "version.h"

#include <gdk/gdkx.h>

#include "gtkntf_event.h"
#include "gtkntf_event_info.h"
#include "gtkntf_gtk_utils.h"
#include "gtkntf_item.h"
#include "gtkntf_item_text.h"
#include "gtkntf_notification.h"
#include "gtkntf_theme.h"
#include "gtkntf_theme_ops.h"
#include "gtkpref_notify.h"

struct _GtkNtfItemText{
	GtkNtfItem *item;
	gchar *format;
	gchar *font;
	gchar *color;
	GtkNtfItemTextClipping clipping;
	gint width;
};


#define IS_EVEN(number) ((number & 1) == 1 ? FALSE: TRUE)

/*******************************************************************************
 * Subsystem
 *
 * If I read the mail thread correctly, a ft2 font map caches the glyphs.
 * I was disregarding the font map and everything because I only needed it to
 * create the context in order to create the layout, this was causing some
 * major memory issues.  Therefore the gtkntf_item_text subsystem was created.  All
 * this does is hold our font map and context.  On win32 we do not need a font
 * map so all it does is keep the context around until uninit is called.  With
 * the caching in ft2, we should in theory be drawing anything thats not a
 * format token faster, which is a good thing.  Also, we aren't constantly
 * getting the context and unref'n it which will save a few cycles as well.
 ******************************************************************************/
static PangoFontMap *map = NULL;
static PangoContext *context = NULL;

void
gtkntf_item_text_init() {
#ifndef _WIN32
	gdouble xdpi = 75, ydpi = 75;
#	if GTK_CHECK_VERSION(2,2,0)
	GdkDisplay *display;
	GdkScreen *screen;
#	endif
#endif

	map = pango_ft2_font_map_new();

#ifndef _WIN32
#	if GTK_CHECK_VERSION(2,2,0)
	display = gdk_display_get_default();
	screen = gdk_display_get_screen(display, oul_prefs_get_int(BEASY_PREFS_NTF_ADVANCED_SCREEN));
	xdpi = (double)((float)gdk_screen_get_width(screen) / (float)gdk_screen_get_width_mm(screen) * 25.4);
	ydpi = (double)((float)gdk_screen_get_height(screen) / (float)gdk_screen_get_height_mm(screen) * 25.4);
#	endif
	pango_ft2_font_map_set_resolution(PANGO_FT2_FONT_MAP(map), xdpi, ydpi);
#endif
	context = pango_ft2_font_map_create_context(PANGO_FT2_FONT_MAP(map));
}

void
gtkntf_item_text_uninit() {
	if(map)
		g_object_unref(G_OBJECT(map));
	if(context)
		g_object_unref(G_OBJECT(context));
}

/******************************************************************************* 
 * API
 ******************************************************************************/
void
gtkntf_item_text_destroy(GtkNtfItemText *item_text) {
	g_return_if_fail(item_text);

	item_text->item = NULL;

	if(item_text->format) {
		g_free(item_text->format);
		item_text->format = NULL;
	}

	if(item_text->font) {
		g_free(item_text->font);
		item_text->font = NULL;
	}

	if(item_text->color) {
		g_free(item_text->color);
		item_text->color = NULL;
	}

	item_text->clipping = GTKNTF_ITEM_TEXT_CLIPPING_UNKNOWN;
	item_text->width = 0;

	g_free(item_text);
	item_text = NULL;
}

GtkNtfItemText *
gtkntf_item_text_new(GtkNtfItem *item) {
	GtkNtfItemText *item_text;

	g_return_val_if_fail(item, NULL);

	item_text = g_new0(GtkNtfItemText, 1);

	item_text->item = item;

	return item_text;
}

static GtkNtfItemTextClipping
text_clipping_from_string(const gchar *string) {
	g_return_val_if_fail(string, GTKNTF_ITEM_TEXT_CLIPPING_UNKNOWN);

	if(!g_ascii_strcasecmp(string, "truncate"))
		return GTKNTF_ITEM_TEXT_CLIPPING_TRUNCATE;
	if(!g_ascii_strcasecmp(string, "ellipsis-start"))
		return GTKNTF_ITEM_TEXT_CLIPPING_ELLIPSIS_START;
	if(!g_ascii_strcasecmp(string, "ellipsis-middle"))
		return GTKNTF_ITEM_TEXT_CLIPPING_ELLIPSIS_MIDDLE;
	if(!g_ascii_strcasecmp(string, "ellipsis-end"))
		return GTKNTF_ITEM_TEXT_CLIPPING_ELLIPSIS_END;
	else
		return GTKNTF_ITEM_TEXT_CLIPPING_UNKNOWN;
}

static const gchar *
text_clipping_to_string(GtkNtfItemTextClipping clip) {
	g_return_val_if_fail(clip != GTKNTF_ITEM_TEXT_CLIPPING_UNKNOWN, NULL);

	switch(clip) {
		case GTKNTF_ITEM_TEXT_CLIPPING_TRUNCATE:
			return "truncate";
			break;
		case GTKNTF_ITEM_TEXT_CLIPPING_ELLIPSIS_START:
			return "ellipsis-start";
			break;
		case GTKNTF_ITEM_TEXT_CLIPPING_ELLIPSIS_MIDDLE:
			return "ellipsis-middle";
			break;
		case GTKNTF_ITEM_TEXT_CLIPPING_ELLIPSIS_END:
			return "ellipsis-end";
			break;
		case GTKNTF_ITEM_TEXT_CLIPPING_UNKNOWN:
		default:
			return NULL;
			break;
	}
}

GtkNtfItemText *
gtkntf_item_text_new_from_xmlnode(GtkNtfItem *item, xmlnode *node) {
	GtkNtfItemText *item_text;
	const gchar *data = NULL;

	g_return_val_if_fail(item, NULL);
	g_return_val_if_fail(node, NULL);

	item_text = gtkntf_item_text_new(item);

	if(!(data = xmlnode_get_attrib(node, "format"))) {
		oul_debug_info("GTKNotify", "** Error loading text item: 'No format given'\n");
		gtkntf_item_text_destroy(item_text);
		return NULL;
	}
	item_text->format = g_strdup(data);

	if((data = xmlnode_get_attrib(node, "font")))
		item_text->font = g_strdup(data);

	if((data = xmlnode_get_attrib(node, "color")))
		item_text->color = g_strdup(data);

	data = xmlnode_get_attrib(node, "clipping");
	item_text->clipping = text_clipping_from_string(data);
	if(item_text->clipping == GTKNTF_ITEM_TEXT_CLIPPING_UNKNOWN) {
		oul_debug_info("GTKNotify", "** Error loading text item: "
						"'Unknown clipping type'\n");
		gtkntf_item_destroy(item);
		return NULL;
	}

	data = xmlnode_get_attrib(node, "width");
	if(data)
		item_text->width = atoi(data);
	else
		item_text->width = 0;

	return item_text;
}

GtkNtfItemText *
gtkntf_item_text_copy(GtkNtfItemText *text) {
	GtkNtfItemText *new_text;

	g_return_val_if_fail(text, NULL);

	new_text = gtkntf_item_text_new(text->item);

	if(text->format)
		new_text->format = g_strdup(text->format);

	if(text->font)
		new_text->font = g_strdup(text->font);

	if(text->color)
		new_text->color = g_strdup(text->color);

	new_text->clipping = text->clipping;
	new_text->width = text->width;

	return new_text;
}

xmlnode *
gtkntf_item_text_to_xmlnode(GtkNtfItemText *text) {
	xmlnode *parent;

	parent = xmlnode_new("text");

	if(text->format)
		xmlnode_set_attrib(parent, "format", text->format);

	if(text->font)
		xmlnode_set_attrib(parent, "font", text->font);

	if(text->color)
		xmlnode_set_attrib(parent, "color", text->color);

	if(text->clipping != GTKNTF_ITEM_TEXT_CLIPPING_UNKNOWN)
		xmlnode_set_attrib(parent, "clipping", text_clipping_to_string(text->clipping));

	if(text->width >= 0) {
		gchar *width = g_strdup_printf("%d", text->width);
		xmlnode_set_attrib(parent, "width", width);
		g_free(width);
	}

	return parent;
}

void
gtkntf_item_text_set_format(GtkNtfItemText *item_text, const gchar *format) {
	g_return_if_fail(item_text);
	g_return_if_fail(format);

	if(item_text->format)
		g_free(item_text->format);

	item_text->format = g_strdup(format);
}

const gchar *
gtkntf_item_text_get_format(GtkNtfItemText *item_text) {
	g_return_val_if_fail(item_text, NULL);

	return item_text->format;
}

void
gtkntf_item_text_set_font(GtkNtfItemText *item_text, const gchar *font) {
	g_return_if_fail(item_text);
	g_return_if_fail(font);

	if(item_text->font)
		g_free(item_text->font);

	item_text->font = g_strdup(font);
}

const gchar *
gtkntf_item_text_get_font(GtkNtfItemText *item_text) {
	g_return_val_if_fail(item_text, NULL);

	return item_text->font;
}

void
gtkntf_item_text_set_color(GtkNtfItemText *item_text, const gchar *color) {
	g_return_if_fail(item_text);
	g_return_if_fail(color);

	if(item_text->color)
		g_free(item_text->color);

	item_text->color = g_strdup(color);
}

const gchar *
gtkntf_item_text_get_color(GtkNtfItemText *item_text) {
	g_return_val_if_fail(item_text, NULL);

	return item_text->color;
}

void
gtkntf_item_text_set_clipping(GtkNtfItemText *item_text, GtkNtfItemTextClipping clipping) {
	g_return_if_fail(item_text);
	g_return_if_fail(clipping >= 0 || clipping < GTKNTF_ITEM_TEXT_CLIPPING_UNKNOWN);

	item_text->clipping = clipping;
}

GtkNtfItemTextClipping
gtkntf_item_text_get_clipping(GtkNtfItemText *item_text) {
	g_return_val_if_fail(item_text, GTKNTF_ITEM_TEXT_CLIPPING_UNKNOWN);

	return item_text->clipping;
}

void
gtkntf_item_text_set_width(GtkNtfItemText *item_text, gint width) {
	g_return_if_fail(item_text);
	g_return_if_fail(width >= 0);

	item_text->width = width;
}

gint
gtkntf_item_text_get_width(GtkNtfItemText *item_text) {
	g_return_val_if_fail(item_text, -1);

	return item_text->width;
}

void
gtkntf_item_text_set_item(GtkNtfItemText *item_text, GtkNtfItem *item) {
	g_return_if_fail(item_text);
	g_return_if_fail(item);

	item_text->item = item;
}

GtkNtfItem *
gtkntf_item_text_get_item(GtkNtfItemText *item_text) {
	g_return_val_if_fail(item_text, NULL);

	return item_text->item;
}

/*******************************************************************************
 * Rendering stuff
 ******************************************************************************/
/* why did I think this was a good idea? */
static gchar *
gtkntf_item_text_parse_format(GtkNtfItemText *item_text, GtkNtfEventInfo *info) {
	GtkNtfEvent *event;
	GtkNtfNotification *notification;
	GtkNtfTheme *theme;
	GtkNtfThemeOptions *ops;
	GString *str;
	gchar *ret;
	const gchar *tokens, *format, *time_format, *date_format, *warning;
	const gchar *source, *title, *content;
	time_t rtime;
	static char buff[80];
	struct tm *ltime;

	g_return_val_if_fail(item_text, NULL);
	g_return_val_if_fail(info, NULL);

	format = item_text->format;

	notification = gtkntf_item_get_notification(item_text->item);
	theme = gtkntf_notification_get_theme(notification);
	ops = gtkntf_theme_get_theme_options(theme);

	time_format = gtkntf_theme_options_get_time_format(ops);
	date_format = gtkntf_theme_options_get_date_format(ops);
	warning = gtkntf_theme_options_get_warning(ops);

	event = gtkntf_event_info_get_event(info);
	source = gtkntf_event_info_get_source(info);
	title = gtkntf_event_info_get_title(info);
	content = gtkntf_event_info_get_content(info);


	str = g_string_new("");

	tokens = gtkntf_event_get_tokens(event);
	time(&rtime);
	ltime = localtime(&rtime);

	while(format && format[0]) {
		if(format[0] == '\\') {
			format++;
			continue;
		}

		if(format[0] != '%') {
			str = g_string_append_c(str, format[0]);
			format++;
			continue;
		}

		/* this increment is to get past the % */
		format++;

		if(!format[0])
			break;

		if(!strchr(tokens, format[0])) {
			format++;
			continue;
		}

		switch(format[0]) {
			case '%': /* % */
				str = g_string_append_c(str, '%');
				break;
			case 'Y': /* four digit year */
				strftime(buff, sizeof(buff), "%Y", ltime);
				str = g_string_append(str, buff);
				break;
			case 'y': { /* two digit year */
				/* dirty hack to avoid compiler warning */
				const gchar *fmt = "%y";
				strftime(buff, sizeof(buff), fmt, ltime);
				str = g_string_append(str, buff);
				break;
			case 'M': /* month */
                strftime(buff, sizeof(buff), "%m", ltime);
				str = g_string_append(str, buff);
				break;
			case 'D': /* date */
				strftime(buff, sizeof(buff), date_format, ltime);
				str = g_string_append(str, buff);
				break;
			case 'd': /* day 01-31 */
				strftime(buff, sizeof(buff), "%d", ltime);
				str = g_string_append(str, buff);
				break;
			case 'H': /* hour 01-23 */
				strftime(buff, sizeof(buff), "%H", ltime);
				str = g_string_append(str, buff);
				break;
			case 'h': /* hour 01-12 */
				strftime(buff, sizeof(buff), "%I", ltime);
				str = g_string_append(str, buff);
				break;
			case 'm': /* minute */
				strftime(buff, sizeof(buff), "%M", ltime);
				str = g_string_append(str, buff);
				break;
			case 's': /* seconds 00-59 */
				strftime(buff, sizeof(buff), "%S", ltime);
				str = g_string_append(str, buff);
				break;
			case 'T': /* Time according to the theme var */
				strftime(buff, sizeof(buff), time_format, ltime);
				str = g_string_append(str, buff);
				break;
			case 't': /* seconds since the epoc */
				strftime(buff, sizeof(buff), "%s", ltime);
				str = g_string_append(str, buff);
				break;
			case 'S': /* source */
				str = g_string_append(str, source);
			case 'R': /* title */
				if(title)
					str = g_string_append(str, title);
				break;
			case 'r': /* content */
				if(content)
					str = g_string_append(str, content);
				break;
			} default:
				break;
		}

		/* this increment is to get past the formatting char */
		format++;
	}

	ret = str->str;
	g_string_free(str, FALSE);

	return ret;
}

/* ugly hack to keep us working on glib 2.0 */
#if !GLIB_CHECK_VERSION(2,2,0)
static gchar *
g_utf8_strreverse(const gchar *str, gssize len) {
	gchar *result;
	const gchar *p;
	gchar *m, *r, skip;

	if (len < 0)
		len = strlen(str);

	result = g_new0(gchar, len + 1);
	r = result + len;
	p = str;
	while (*p) {
		skip = g_utf8_skip[*(guchar*)p];
		r -= skip;
		for (m = r; skip; skip--)
			*m++ = *p++;
	}

	return result;
}
#endif

/* this will probably break.. be sure to test it!!! */
static gchar *
gtkntf_utf8_strrndup(const gchar *text, gint n) {
	gchar *rev = NULL, *tmp = NULL;

	rev = g_utf8_strreverse(text, -1);
	/* ewww, what's going on here? */
	/* oh, I remember... */
	/* tell me then? */
	/* well, g_utf8_strncpy doesn't allocate, so we use strdup to allocate for us */
	/* oh neat, that's pretty ugly though */
	/* yeah, there's probably a better way... */
	tmp = g_strdup(rev);
	tmp = g_utf8_strncpy(tmp, rev, n);
	g_free(rev);
	rev = g_utf8_strreverse(tmp, -1);
	g_free(tmp);

	return rev;
}

/*******************************************************************************
 * The dreaded text clipping functions
 *
 * Hopefully now "utf8 safe" (tm)
 ******************************************************************************/
static void
text_truncate(PangoLayout *layout, gint width, gint offset) {
	const gchar *text;
	gchar *new_text = NULL;
	gint l_width = 0;

	g_return_if_fail(layout);

	while(1) {
		pango_layout_get_pixel_size(layout, &l_width, NULL);

		if(l_width + offset <= width)
			break;

		text = pango_layout_get_text(layout);
		new_text = g_strdup(text);
		new_text = g_utf8_strncpy(new_text, text, g_utf8_strlen(text, -1) - 1);
		pango_layout_set_text(layout, new_text, -1);
		g_free(new_text);
	}
}

static void
text_ellipsis_start(PangoLayout *layout, gint width, gint offset,
					const gchar *ellipsis_text, gint ellipsis_width)
{
	const gchar *text;
	gchar *new_text = NULL;
	gint l_width = 0;

	g_return_if_fail(layout);

	while(1) {
		pango_layout_get_pixel_size(layout, &l_width, NULL);

		if(l_width + offset + ellipsis_width <= width)
			break;

		text = pango_layout_get_text(layout);
		new_text = gtkntf_utf8_strrndup(text, g_utf8_strlen(text, -1) - 1);
		pango_layout_set_text(layout, new_text, -1);
		g_free(new_text);
	}

	text = pango_layout_get_text(layout);
	new_text = g_strdup_printf("%s%s", ellipsis_text, text);
	pango_layout_set_text(layout, new_text, -1);
	g_free(new_text);
}

static void
text_ellipsis_middle(PangoLayout *layout, gint width, gint offset,
					 const gchar *ellipsis_text, gint ellipsis_width)
{
	const gchar *text;
	gchar *new_text = NULL, *left_text = NULL, *right_text = NULL;
	gint l_width = 0, mid;

	g_return_if_fail(layout);

	while(1) {
		pango_layout_get_pixel_size(layout, &l_width, NULL);

		if(l_width + offset + ellipsis_width <= width)
			break;

		text = pango_layout_get_text(layout);
		mid = g_utf8_strlen(text, -1) / 2;

		left_text = g_strdup(text);
		left_text = g_utf8_strncpy(left_text, text, mid);

		if(IS_EVEN(g_utf8_strlen(text, -1)))
			right_text = gtkntf_utf8_strrndup(text, mid - 1);
		else
			right_text = gtkntf_utf8_strrndup(text, mid);

		new_text = g_strdup_printf("%s%s", left_text, right_text);
		g_free(left_text);
		g_free(right_text);

		pango_layout_set_text(layout, new_text, -1);
		g_free(new_text);
	}

	text = pango_layout_get_text(layout);
	mid = g_utf8_strlen(text, -1) / 2;

	left_text = g_strdup(text);
	left_text = g_utf8_strncpy(left_text, text, mid);
	if(IS_EVEN(g_utf8_strlen(text, -1)))
		right_text = gtkntf_utf8_strrndup(text, mid - 1);
	else
		right_text = gtkntf_utf8_strrndup(text, mid);

	new_text = g_strdup_printf("%s%s%s", left_text, ellipsis_text, right_text);
	g_free(left_text);
	g_free(right_text);

	pango_layout_set_text(layout, new_text, -1);
	g_free(new_text);
}

static void
text_ellipsis_end(PangoLayout *layout, gint width, gint offset,
				  const gchar *ellipsis_text, gint ellipsis_width)
{
	const gchar *text;
	gchar *new_text = NULL;
	gint l_width = 0;

	g_return_if_fail(layout);

	while(1) {
		pango_layout_get_pixel_size(layout, &l_width, NULL);

		if(l_width + offset + ellipsis_width <= width)
			break;

		text = pango_layout_get_text(layout);
		new_text = g_strdup(text);
		new_text = g_utf8_strncpy(new_text, text, g_utf8_strlen(text, -1) - 1);
		pango_layout_set_text(layout, new_text, -1);
		g_free(new_text);
	}

	text = pango_layout_get_text(layout);
	new_text = g_strdup_printf("%s%s", text, ellipsis_text);
	pango_layout_set_text(layout, new_text, -1);
	g_free(new_text);
}

static void
gtkntf_item_text_clip(GtkNtfItemText *item_text, PangoLayout *layout,
				  gint pixbuf_width)
{
	GtkNtfNotification *notification;
	GtkNtfTheme *theme;
	GtkNtfThemeOptions *ops;
	GtkNtfItemOffset *ioffset;
	PangoLayout *ellipsis;
	const gchar *ellipsis_text;
	gint e_width = 0, l_width = 0, width = 0, offset = 0;

	g_return_if_fail(item_text);
	g_return_if_fail(layout);

	notification = gtkntf_item_get_notification(item_text->item);
	theme = gtkntf_notification_get_theme(notification);
	ops = gtkntf_theme_get_theme_options(theme);

	ellipsis_text = gtkntf_theme_options_get_ellipsis(ops);

	if((ioffset = gtkntf_item_get_horz_offset(item_text->item))) {
		if(ioffset && gtkntf_item_offset_get_is_percentage(ioffset))
			offset = (pixbuf_width * gtkntf_item_offset_get_value(ioffset)) / 100;
		else
			offset = gtkntf_item_offset_get_value(ioffset);
	} else {
		offset = 0;
	}

	width = item_text->width;
	if(width == 0)
		width = pixbuf_width;
	else
		offset = 0;

	ellipsis = pango_layout_copy(layout);
	pango_layout_set_text(ellipsis, ellipsis_text, -1);
	pango_layout_get_pixel_size(ellipsis, &e_width, NULL);
	g_object_unref(G_OBJECT(ellipsis));

	pango_layout_get_pixel_size(layout, &l_width, NULL);
	if(l_width <= width)
		return;

	switch (item_text->clipping) {
		case GTKNTF_ITEM_TEXT_CLIPPING_ELLIPSIS_START:
			text_ellipsis_start(layout, width, offset, ellipsis_text, e_width);
			break;
		case GTKNTF_ITEM_TEXT_CLIPPING_ELLIPSIS_MIDDLE:
			text_ellipsis_middle(layout, width, offset, ellipsis_text, e_width);
			break;
		case GTKNTF_ITEM_TEXT_CLIPPING_ELLIPSIS_END:
			text_ellipsis_end(layout, width, offset, ellipsis_text, e_width);
			break;
		case GTKNTF_ITEM_TEXT_CLIPPING_TRUNCATE:
		default:
			text_truncate(layout, width, offset);
			break;
	}
}

static PangoLayout *
gtkntf_item_text_create_layout(GtkNtfItemText *item_text, GtkNtfEventInfo *info, gint width)
{
	PangoLayout *layout = NULL;
	PangoFontDescription *font = NULL;
	gchar *text = NULL;

	g_return_val_if_fail(item_text, NULL);
	g_return_val_if_fail(info, NULL);

	layout = pango_layout_new(context);
	pango_layout_set_width(layout, -1);

	if(item_text->font) {
		font = pango_font_description_from_string(item_text->font);
		pango_layout_set_font_description(layout, font);
		pango_font_description_free(font);
	} else {
		pango_layout_set_font_description(layout, gtkntf_gtk_theme_get_font());
	}

	text = gtkntf_item_text_parse_format(item_text, info);
	pango_layout_set_text(layout, text, -1);
	g_free(text);

	gtkntf_item_text_clip(item_text, layout, width);

	return layout;
}

/* This function has cost me 5 days of trying to find some way to make gtk/pango/gdk do
 * this.  I'm only using this way because 2 gtk/pango developers said this is the only
 * way at this time. So if you change it.. Your changes better fucking work :P
 */
static GdkPixbuf *
gtkntf_pixbuf_new_from_ft2_bitmap(FT_Bitmap *bitmap, PangoColor *color) {
	GdkPixbuf *pixbuf;
	guchar *buffer, *pbuffer;
	gint w, h, rowstride;
	guint8 r, g, b, *alpha;

	/* Grab the colors from the PangoColor and shift 8 bits because we're only in 8 bit
	 * mode, and a PangoColor's elements are guint16's which are 16 bits...
	 */
	r = color->red >> 8;
	g = color->green >> 8;
	b = color->blue >> 8;

	/* create the new pixbuf */
	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
							bitmap->width, bitmap->rows);
	if(!pixbuf)
		return NULL;

	/* clear out the pixbuf to transparent black */
	gdk_pixbuf_fill(pixbuf, 0x00000000);

	buffer = gdk_pixbuf_get_pixels(pixbuf);
	rowstride = gdk_pixbuf_get_rowstride(pixbuf);

	/* ok here's the run down...
	 *
	 * From devhelp:
	 *  Image data in a pixbuf is stored in memory in uncompressed, packed format. Rows
	 *  in the image are stored top to bottom, and in each row pixels are stored from
	 *  left to right. There may be padding at the end of a row. The "rowstride" value of
	 *  a pixbuf, as returned by gdk_pixbuf_get_rowstride(), indicates the number of
	 *  bytes between rows.
	 *
	 * So we take the height of the FT_Bitmap (the text), and increment until we're done.
	 * simple enough.
	 *
	 * Then we get the alpha for the row in the FT_Bitmap
	 *
	 * Next we move left to right and draw accordingly.
	 *
	 * Repeat until we've gone through the whole FT_Bitmap.
	 */
	for(h = 0; h < bitmap->rows; h++) {
		pbuffer = buffer + (h * rowstride);
		/* get the alpha from the FT_Bitmap */
		alpha = bitmap->buffer + (h * (bitmap->pitch));
		for(w = 0; w < bitmap->width; w++) {
			*pbuffer++ = r;
			*pbuffer++ = g;
			*pbuffer++ = b;
			*pbuffer++ = *alpha++;
		}
	}

	return pixbuf;
}

void
gtkntf_item_text_render(GtkNtfItemText *item_text, GdkPixbuf *pixbuf, GtkNtfEventInfo *info)
{
	GdkPixbuf *t_pixbuf = NULL;
	PangoColor color;
	PangoLayout *layout = NULL;
	FT_Bitmap bitmap;
	gint x = 0, y = 0;
	gint n_width = 0, n_height = 0;
	gint t_width = 0, t_height = 0;
	gint l_width = 0, l_height = 0;

	g_return_if_fail(item_text);
	g_return_if_fail(pixbuf);
	g_return_if_fail(info);

	/* get the width and height of the notification pixbuf */
	n_width = gdk_pixbuf_get_width(pixbuf);
	n_height = gdk_pixbuf_get_height(pixbuf);

	/* create the layout */
	layout = gtkntf_item_text_create_layout(item_text, info, n_width);

	if(!layout)
		return;

	/* setup the FT_BITMAP */
	pango_layout_get_pixel_size(layout, &l_width, &l_height);
	bitmap.rows = l_height;
	bitmap.width = l_width;
	bitmap.pitch = (bitmap.width + 3) & ~3;
	bitmap.buffer = g_new0(guint8, bitmap.rows * bitmap.pitch);
	bitmap.num_grays = 255;
	bitmap.pixel_mode = ft_pixel_mode_grays;
	pango_ft2_render_layout(&bitmap, layout, 0, 0);
	g_object_unref(G_OBJECT(layout));

	if(!item_text->color) {
		GdkColor g_color;

		gtkntf_gtk_theme_get_fg_color(&g_color);
		gtkntf_gtk_color_pango_from_gdk(&color, &g_color);
	} else if(!pango_color_parse(&color, item_text->color))
		color.red = color.green = color.blue = 0;

	t_pixbuf = gtkntf_pixbuf_new_from_ft2_bitmap(&bitmap, &color);
	g_free(bitmap.buffer);

	if(!t_pixbuf)
		return;

	t_width = gdk_pixbuf_get_width(t_pixbuf);
	t_height = gdk_pixbuf_get_height(t_pixbuf);

	gtkntf_item_get_render_position(&x, &y, t_width, t_height, n_width, n_height,
								item_text->item);

	gtkntf_gtk_pixbuf_clip_composite(t_pixbuf, x, y, pixbuf);

	g_object_unref(G_OBJECT(t_pixbuf));
}
