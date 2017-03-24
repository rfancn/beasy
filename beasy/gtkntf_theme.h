#ifndef GTKNTF_THEME_H
#define GTKNTF_THEME_H

typedef struct _GtkNtfTheme GtkNtfTheme;

#define GTKNTF_THEME(theme) 		((GtkNtfTheme *)theme)
#define GTKNTF_THEME_API_VERSION 	(1)


#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gtkntf_event_info.h"
#include "gtkntf_item.h"
#include "gtkntf_theme_info.h"
#include "gtkntf_theme_ops.h"

G_BEGIN_DECLS

GtkNtfTheme *gtkntf_theme_new();
GtkNtfTheme *gtkntf_theme_new_from_file(const gchar *filename);
void gtkntf_theme_destory(GtkNtfTheme *theme);

GtkNtfTheme *gtkntf_theme_find_theme_by_name(const gchar *name);
gboolean gtkntf_theme_save_to_file(GtkNtfTheme *theme, const gchar *filename);
GtkNtfTheme *gtkntf_theme_find_theme_by_filename(const gchar *filename);
gchar *gtkntf_theme_strip_name(GtkNtfTheme *theme);

gboolean gtkntf_theme_is_loaded(const gchar *filename);
gboolean gtkntf_theme_is_probed(const gchar *filename);
void gtkntf_theme_load(const gchar *filename);
void gtkntf_theme_unload(GtkNtfTheme *theme);
void gtkntf_theme_probe(const gchar *filename);
void gtkntf_theme_unprobe(const gchar *filename);
void gtkntf_themes_probe();
void gtkntf_themes_unprobe();

gint gtkntf_theme_get_api_version(GtkNtfTheme *theme);
const gchar *gtkntf_theme_get_filename(GtkNtfTheme *theme);
const gchar *gtkntf_theme_get_path(GtkNtfTheme *theme);
GtkNtfNotification *gtkntf_theme_get_master(GtkNtfTheme *theme);
void gtkntf_theme_set_master(GtkNtfTheme *theme, GtkNtfNotification *notification);
gchar *gtkntf_theme_get_supported_notifications(GtkNtfTheme *theme);
void gtkntf_theme_set_theme_info(GtkNtfTheme *theme, GtkNtfThemeInfo *info);
GtkNtfThemeInfo *gtkntf_theme_get_theme_info(GtkNtfTheme *theme);
void gtkntf_theme_set_theme_options(GtkNtfTheme *theme, GtkNtfThemeOptions *ops);
GtkNtfThemeOptions *gtkntf_theme_get_theme_options(GtkNtfTheme *theme);

void gtkntf_theme_add_notification(GtkNtfTheme *theme, GtkNtfNotification *notification);
void gtkntf_theme_remove_notification(GtkNtfTheme *theme, GtkNtfNotification *notification);
GList *gtkntf_theme_get_notifications(GtkNtfTheme *theme);

GList *gtkntf_themes_get_all();
GList *gtkntf_themes_get_loaded();
void gtkntf_themes_unload();
void gtkntf_themes_save_loaded();
void gtkntf_themes_load_saved();

G_END_DECLS

#endif /* GTKNTF_THEME_H */
