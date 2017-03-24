#ifndef GTKNTF_GTK_UTILS_H
#define GTKNTF_GTK_UTILS_H

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <pango/pango.h>

G_BEGIN_DECLS

/* Color stuff */
guint32 gtkntf_gtk_color_pixel_from_gdk(const GdkColor *color);
void 	gtkntf_gtk_color_pango_from_gdk(PangoColor *pango, const GdkColor *gdk);
void 	gtkntf_gtk_color_gdk_from_pango(GdkColor *gdk, const PangoColor *pango);

/* Gtk Theme Stuff */
void 					gtkntf_gtk_theme_get_bg_color(GdkColor *color);
void 					gtkntf_gtk_theme_get_fg_color(GdkColor *color);
GdkPixmap *				gtkntf_gtk_theme_get_bg_pixmap();
PangoFontDescription *	gtkntf_gtk_theme_get_font();

/* Pixbuf Stuff */
void gtkntf_gtk_pixbuf_tile(GdkPixbuf *dest, const GdkPixbuf *tile);
void gtkntf_gtk_pixbuf_clip_composite(const GdkPixbuf *src, gint x, gint y, GdkPixbuf *dest);

/* Subsystem */
void gtkntf_gtk_utils_init();
void gtkntf_gtk_utils_uninit();

G_END_DECLS

#endif /* GTKNTF_GTK_UTILS_H */
