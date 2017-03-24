#ifndef GTKNTF_THEME_INFO_H
#define GTKNTF_THEME_INFO_H

typedef struct _GtkNtfThemeInfo GtkNtfThemeInfo;

#define GTKNTF_THEME_INFO(obj)	((GtkNtfThemeInfo *)(obj))

#include <glib.h>
#include "xmlnode.h"


G_BEGIN_DECLS

GtkNtfThemeInfo *	gtkntf_theme_info_new();
GtkNtfThemeInfo *	gtkntf_theme_info_new_from_xmlnode(xmlnode *node);
xmlnode *			gtkntf_theme_info_to_xmlnode(GtkNtfThemeInfo *info);
void 				gtkntf_theme_info_destroy(GtkNtfThemeInfo *info);

gchar *				gtkntf_theme_info_strip_name(GtkNtfThemeInfo *info);

void 				gtkntf_theme_info_set_name(GtkNtfThemeInfo *info, const gchar *name);
const gchar *		gtkntf_theme_info_get_name(GtkNtfThemeInfo *info);
void 				gtkntf_theme_info_set_version(GtkNtfThemeInfo *info, const gchar *version);
const gchar *		gtkntf_theme_info_get_version(GtkNtfThemeInfo *info);
void 				gtkntf_theme_info_set_summary(GtkNtfThemeInfo *info, const gchar *summary);
const gchar *		gtkntf_theme_info_get_summary(GtkNtfThemeInfo *info);
void 				gtkntf_theme_info_set_description(GtkNtfThemeInfo *info, const gchar *description);
const gchar *		gtkntf_theme_info_get_description(GtkNtfThemeInfo *info);
void 				gtkntf_theme_info_set_author(GtkNtfThemeInfo *info, const gchar *author);
const gchar *		gtkntf_theme_info_get_author(GtkNtfThemeInfo *info);
void 				gtkntf_theme_info_set_website(GtkNtfThemeInfo *info, const gchar *author);
const gchar *		gtkntf_theme_info_get_website(GtkNtfThemeInfo *info);

G_END_DECLS

#endif
