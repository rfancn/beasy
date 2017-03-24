#include "debug.h"

#include "internal.h"
#include "gtkntf_gtk_utils.h"

/******************************************************************************
 * Globals
 *****************************************************************************/
static GtkWidget *style_widget = NULL;

/******************************************************************************
 * API
 *****************************************************************************/
/* Color stuff */
guint32
gtkntf_gtk_color_pixel_from_gdk(const GdkColor *color) {
	guint32 pixel;

	g_return_val_if_fail(color, 0);

	pixel = (((color->red & 0xFF00) << 8) | ((color->green & 0xFF00)) |
			 ((color->blue & 0xFF00) >> 8)) << 8;
	return pixel;
}

void 
gtkntf_gtk_color_pango_from_gdk(PangoColor *pango, const GdkColor *gdk) {
	g_return_if_fail(pango);
	g_return_if_fail(gdk);

	pango->red = gdk->red;
	pango->green = gdk->green;
	pango->blue = gdk->blue;
}

void
gtkntf_gtk_color_gdk_from_pango(GdkColor *gdk, const PangoColor *pango) {
	g_return_if_fail(gdk);
	g_return_if_fail(pango);

	gdk->red = pango->red;
	gdk->green = pango->green;
	gdk->blue = pango->blue;
}

/******************************************************************************
 * Theme stuff
 *****************************************************************************/
void
gtkntf_gtk_theme_get_bg_color(GdkColor *color) {
	GtkStyle *style;

	g_return_if_fail(color);

	style = gtk_rc_get_style(style_widget);
	*color = style->bg[GTK_STATE_NORMAL];
}

void
gtkntf_gtk_theme_get_fg_color(GdkColor *color) {
	GtkStyle *style;

	g_return_if_fail(color);

	style = gtk_rc_get_style(style_widget);
	*color = style->fg[GTK_STATE_NORMAL];
}

GdkPixmap *
gtkntf_gtk_theme_get_bg_pixmap() {
	GtkStyle *style;

	style = gtk_rc_get_style(style_widget);

	return style->bg_pixmap[GTK_STATE_NORMAL];
}

PangoFontDescription *
gtkntf_gtk_theme_get_font() {
	GtkStyle *style;

	style = gtk_rc_get_style(style_widget);

	if(!pango_font_description_get_family(style->font_desc))
		pango_font_description_set_family(style->font_desc, "Sans");

	if(pango_font_description_get_size(style->font_desc) <= 0)
		pango_font_description_set_size(style->font_desc, 10 * PANGO_SCALE);

	return style->font_desc;
}

/******************************************************************************
 * Pixbuf stuff
 *****************************************************************************/
void
gtkntf_gtk_pixbuf_tile(GdkPixbuf *dest, const GdkPixbuf *tile) {
	gint dest_width, dest_height;
	gint tile_width, tile_height;
	gint copy_width, copy_height;
	gint x, y;

	g_return_if_fail(dest);
	g_return_if_fail(tile);

	dest_width = gdk_pixbuf_get_width(dest);
	dest_height = gdk_pixbuf_get_height(dest);

	tile_width = gdk_pixbuf_get_width(tile);
	tile_height = gdk_pixbuf_get_height(tile);

	for(y = 0; y < dest_height; y += tile_height) {
		for(x = 0; x < dest_width; x += tile_width) {
			if(x + tile_width < dest_width)
				copy_width = tile_width;
			else
				copy_width = dest_width - x;

			if(y + tile_height < dest_height)
				copy_height = tile_height;
			else
				copy_height = dest_height - y;

			gdk_pixbuf_copy_area(tile, 0, 0, copy_width, copy_height,
								 dest, x, y);
		}
	}
}

void
gtkntf_gtk_pixbuf_clip_composite(const GdkPixbuf *src, gint x, gint y,
							 GdkPixbuf *dest)
{
	GdkPixbuf *clipped = NULL;
	GdkRectangle clip;
	gint width, height, diff;

	g_return_if_fail(src);
	g_return_if_fail(dest);

	/* grab the dimensions of the destination */
	width = gdk_pixbuf_get_width(dest);
	height = gdk_pixbuf_get_height(dest);

	/* make sure the x is in the background */
	g_return_if_fail(x < width);
	g_return_if_fail(y < height);

	/* setup our clipping rectangle with the default values */
	clip.x = 0;
	clip.y = 0;
	clip.width = gdk_pixbuf_get_width(src);
	clip.height = gdk_pixbuf_get_height(src);

	/* make sure it will paritially show */
	g_return_if_fail(x + clip.width > 0);
	g_return_if_fail(y + clip.height > 0);
	g_return_if_fail(x < width);
	g_return_if_fail(y < height);

	/* do our adjustments */
	if(x < 0) {
		/* this looks goofy but we're adding a negative number,
		 *
		 * ie: it's subtraction..
		 *
		 * I was considering using abs() so it was clearer, but thats another
		 * function call that isn't really necessary.
		 */
		diff = clip.width + x;
		clip.x = clip.width - diff;
		clip.width = diff;
		x = 0;
	}

	if(y < 0) {
		/* see note above.. */
		diff = clip.height + y;
		clip.y = clip.height - diff;
		clip.height = diff;
		y = 0;
	}
	if(x + clip.width > width)
		clip.width = width - (clip.x + x);
	if(y + clip.height > height)
		clip.height = height - (clip.y + y);

	/* a few last sanity checks */
	g_return_if_fail(clip.width > 0);
	g_return_if_fail(clip.height > 0);

	/* create our clipped pixbuf */
	clipped = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
							 clip.width, clip.height);
	g_return_if_fail(clipped);
	gdk_pixbuf_copy_area(src, clip.x, clip.y, clip.width, clip.height,
						 clipped, 0, 0);

	/* composite it */
	gdk_pixbuf_composite(clipped, dest,
						 x, y, clip.width, clip.height,
						 x, y, 1, 1,
						 GDK_INTERP_BILINEAR, 255);

	/* kill the clipped pixbuf */
	g_object_unref(G_OBJECT(clipped));
}

/******************************************************************************
 * Subsystem
 *****************************************************************************/
void
gtkntf_gtk_utils_init() {
	style_widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_name(style_widget, "guifications");
}

void
gtkntf_gtk_utils_uninit() {
	gtk_widget_destroy(style_widget);
}
