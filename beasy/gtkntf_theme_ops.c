#include <glib.h>
#include <string.h>
#include "internal.h"
#include "gtkntf_theme_ops.h"

#include "debug.h"
#include "xmlnode.h"

struct _GtkNtfThemeOptions {
	gchar *date_format;
	gchar *time_format;
	gchar *warning;
	gchar *ellipsis;
};



GtkNtfThemeOptions *
gtkntf_theme_options_new() {
	GtkNtfThemeOptions *ops;

	ops = g_new0(GtkNtfThemeOptions, 1);

	ops->date_format = g_strdup("%x");
	ops->time_format = g_strdup("%X");
	ops->warning = g_strdup("N/A");
	ops->ellipsis = g_strdup("...");

	return ops;
}

GtkNtfThemeOptions *
gtkntf_theme_options_new_from_xmlnode(xmlnode *node) {
	GtkNtfThemeOptions *ops;
	xmlnode *child;
	gchar *data;

	g_return_val_if_fail(node, NULL);

	ops = gtkntf_theme_options_new();

	child = xmlnode_get_child(node, "date_format");
	if(child && (data = xmlnode_get_data(child))) {
		gtkntf_theme_options_set_date_format(ops, data);
		g_free(data);
	}

	child = xmlnode_get_child(node, "time_format");
	if(child && (data = xmlnode_get_data(child))) {
		gtkntf_theme_options_set_time_format(ops, data);
		g_free(data);
	}

	child = xmlnode_get_child(node, "warning");
	if(child && (data = xmlnode_get_data(child))) {
		gtkntf_theme_options_set_warning(ops, data);
		g_free(data);
	}

	child = xmlnode_get_child(node, "ellipsis");
	if(child && (data = xmlnode_get_data(child))) {
		gtkntf_theme_options_set_ellipsis(ops, data);
		g_free(data);
	}

	return ops;
}

xmlnode *
gtkntf_theme_options_to_xmlnode(GtkNtfThemeOptions *ops) {
	xmlnode *parent, *child;

	parent = xmlnode_new("options");

	if(ops->date_format && strlen(ops->date_format)) {
		child = xmlnode_new_child(parent, "date_format");
		xmlnode_insert_data(child, ops->date_format, strlen(ops->date_format));
	}

	if(ops->time_format && strlen(ops->time_format)) {
		child = xmlnode_new_child(parent, "time_format");
		xmlnode_insert_data(child, ops->time_format, strlen(ops->time_format));
	}

	if(ops->warning && strlen(ops->warning)) {
		child = xmlnode_new_child(parent, "warning");
		xmlnode_insert_data(child, ops->warning, strlen(ops->warning));
	}

	if(ops->ellipsis && strlen(ops->ellipsis)) {
		child = xmlnode_new_child(parent, "ellipsis");
		xmlnode_insert_data(child, ops->ellipsis, strlen(ops->ellipsis));
	}

	return parent;
}

void
gtkntf_theme_options_destroy(GtkNtfThemeOptions *ops) {
	g_return_if_fail(ops);

	if(ops->date_format)
		g_free(ops->date_format);

	if(ops->time_format)
		g_free(ops->time_format);

	if(ops->warning)
		g_free(ops->warning);

	if(ops->ellipsis)
		g_free(ops->ellipsis);

	g_free(ops);
	ops = NULL;
}

void
gtkntf_theme_options_set_time_format(GtkNtfThemeOptions *ops, const gchar *format) {
	g_return_if_fail(ops);
	g_return_if_fail(format);

	if(ops->time_format)
		g_free(ops->time_format);

	ops->time_format = g_strdup(format);
}

const gchar *
gtkntf_theme_options_get_time_format(GtkNtfThemeOptions *ops) {
	g_return_val_if_fail(ops, NULL);

	return ops->time_format;
}

void
gtkntf_theme_options_set_date_format(GtkNtfThemeOptions *ops, const gchar *format) {
	g_return_if_fail(ops);
	g_return_if_fail(format);

	if(ops->date_format)
		g_free(ops->date_format);

	ops->date_format = g_strdup(format);
}

const gchar *
gtkntf_theme_options_get_date_format(GtkNtfThemeOptions *ops) {
	g_return_val_if_fail(ops, NULL);

	return ops->date_format;
}

void
gtkntf_theme_options_set_warning(GtkNtfThemeOptions *ops, const gchar *warning) {
	g_return_if_fail(ops);
	g_return_if_fail(warning);

	if(ops->warning)
		g_free(ops->warning);

	ops->warning = g_strdup(warning);
}

const gchar *
gtkntf_theme_options_get_warning(GtkNtfThemeOptions *ops) {
	g_return_val_if_fail(ops, NULL);

	return ops->warning;
}

void
gtkntf_theme_options_set_ellipsis(GtkNtfThemeOptions *ops, const gchar *ellipsis) {
	g_return_if_fail(ops);
	g_return_if_fail(ellipsis);

	if(ops->ellipsis)
		g_free(ops->ellipsis);

	ops->ellipsis = g_strdup(ellipsis);
}

const gchar *
gtkntf_theme_options_get_ellipsis(GtkNtfThemeOptions *ops) {
	g_return_val_if_fail(ops, NULL);

	return ops->ellipsis;
}
