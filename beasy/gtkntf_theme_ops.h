#ifndef GTKNTF_THEME_OPTIONS_H
#define GTKNTF_THEME_OPTIONS_H

typedef struct _GtkNtfThemeOptions GtkNtfThemeOptions;

#define GTKNTF_THEME_OPTIONS(obj)	((GtkNtfThemeOptions *)(obj))

#include <glib.h>

#include "xmlnode.h"


G_BEGIN_DECLS

GtkNtfThemeOptions *gtkntf_theme_options_new();
GtkNtfThemeOptions *gtkntf_theme_options_new_from_xmlnode(xmlnode *node);
xmlnode *gtkntf_theme_options_to_xmlnode(GtkNtfThemeOptions *ops);
void gtkntf_theme_options_destroy(GtkNtfThemeOptions *ops);

void gtkntf_theme_options_set_time_format(GtkNtfThemeOptions *ops, const gchar *format);
const gchar *gtkntf_theme_options_get_time_format(GtkNtfThemeOptions *ops);
void gtkntf_theme_options_set_date_format(GtkNtfThemeOptions *ops, const gchar *format);
const gchar *gtkntf_theme_options_get_date_format(GtkNtfThemeOptions *ops);
void gtkntf_theme_options_set_warning(GtkNtfThemeOptions *ops, const gchar *warning);
const gchar *gtkntf_theme_options_get_warning(GtkNtfThemeOptions *ops);
void gtkntf_theme_options_set_ellipsis(GtkNtfThemeOptions *ops, const gchar *ellipsis);
const gchar *gtkntf_theme_options_get_ellipsis(GtkNtfThemeOptions *ops);

G_END_DECLS

#endif /* GTKNTF_THEME_OPTIONS_H */
