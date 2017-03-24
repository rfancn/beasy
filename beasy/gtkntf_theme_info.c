#include <glib.h>
#include <string.h>

#include "internal.h"
#include "gtkntf_theme_info.h"

#include "debug.h"
#include "xmlnode.h"

struct _GtkNtfThemeInfo {
	gchar *name;
	gchar *version;
	gchar *summary;
	gchar *description;
	gchar *author;
	gchar *website;
};



GtkNtfThemeInfo *
gtkntf_theme_info_new() {
	GtkNtfThemeInfo *info;

	info = g_new0(GtkNtfThemeInfo, 1);

	return info;
}

GtkNtfThemeInfo *
gtkntf_theme_info_new_from_xmlnode(xmlnode *node) {
	GtkNtfThemeInfo *info;
	xmlnode *child;
	gchar *data;

	g_return_val_if_fail(node, NULL);

	info = gtkntf_theme_info_new();

	child = xmlnode_get_child(node, "name");
	if(child && (data = xmlnode_get_data(child))) {
		info->name = g_strdup(data);
		g_free(data);
	}

	child = xmlnode_get_child(node, "version");
	if(child && (data = xmlnode_get_data(child))) {
		info->version = g_strdup(data);
		g_free(data);
	}

	child = xmlnode_get_child(node, "summary");
	if(child && (data = xmlnode_get_data(child))) {
		info->summary = g_strdup(data);
		g_free(data);
	}

	child = xmlnode_get_child(node, "description");
	if(child && (data = xmlnode_get_data(child))) {
		info->description = g_strdup(data);
		g_free(data);
	}

	child = xmlnode_get_child(node, "author");
	if(child && (data = xmlnode_get_data(child))) {
		info->author = g_strdup(data);
		g_free(data);
	}

	child = xmlnode_get_child(node, "website");
	if(child && (data = xmlnode_get_data(child))) {
		info->website = g_strdup(data);
		g_free(data);
	}

	return info;
}

xmlnode *
gtkntf_theme_info_to_xmlnode(GtkNtfThemeInfo *info) {
	xmlnode *parent, *child;

	parent = xmlnode_new("info");

	if(info->name && strlen(info->name)) {
		child = xmlnode_new_child(parent, "name");
		xmlnode_insert_data(child, info->name, strlen(info->name));
	}

	if(info->version && strlen(info->version)) {
		child = xmlnode_new_child(parent, "version");
		xmlnode_insert_data(child, info->version, strlen(info->version));
	}

	if(info->summary && strlen(info->summary)) {
		child = xmlnode_new_child(parent, "summary");
		xmlnode_insert_data(child, info->summary, strlen(info->summary));
	}

	if(info->description && strlen(info->description)) {
		child = xmlnode_new_child(parent, "description");
		xmlnode_insert_data(child, info->description, strlen(info->description));
	}

	if(info->author && strlen(info->author)) {
		child = xmlnode_new_child(parent, "author");
		xmlnode_insert_data(child, info->author, strlen(info->author));
	}

	if(info->website && strlen(info->website)) {
		child = xmlnode_new_child(parent, "website");
		xmlnode_insert_data(child, info->website, strlen(info->website));
	}

	return parent;
}

void
gtkntf_theme_info_destroy(GtkNtfThemeInfo *info) {
	g_return_if_fail(info);

	if(info->name)
		g_free(info->name);

	if(info->version)
		g_free(info->version);

	if(info->summary)
		g_free(info->summary);

	if(info->description)
		g_free(info->description);

	if(info->author)
		g_free(info->author);

	if(info->website)
		g_free(info->website);

	g_free(info);
	info = NULL;
}

gchar *
gtkntf_theme_info_strip_name(GtkNtfThemeInfo *info) {
	GString *str;
	const gchar *iter;
	gchar *dir;

	g_return_val_if_fail(info, NULL);

	if(!info->name)
		return g_strdup("untitled");

	str = g_string_new("");

	iter = info->name;

	if(iter[0] == '.' && strlen(iter) > 1)
		iter++;

	for(; *iter != '\0'; iter++)
	{
		switch(iter[0]) {
			case '\\':
			case '/':
			case ':':
			case '*':
			case '?':
			case '"':
			case '<':
			case '>':
			case '|':
			case '[':
			case ']':
			case '{':
			case '}':
				break;
			case ' ':
				str = g_string_append_c(str, '_');
				break;
			default:
				str = g_string_append_c(str, iter[0]);
		}
	}

	dir = str->str;
	g_string_free(str, FALSE);

	if(!dir)
		dir = g_strdup("untitled");

	return dir;
}

void
gtkntf_theme_info_set_name(GtkNtfThemeInfo *info, const gchar *name) {
	g_return_if_fail(info);
	g_return_if_fail(name);

	if(info->name)
		g_free(info->name);

	info->name = g_strdup(name);
}

const gchar *
gtkntf_theme_info_get_name(GtkNtfThemeInfo *info) {
	g_return_val_if_fail(info, NULL);

	return info->name;
}

void
gtkntf_theme_info_set_version(GtkNtfThemeInfo *info, const gchar *version) {
	g_return_if_fail(info);
	g_return_if_fail(version);

	if(info->version)
		g_free(info->version);

	info->version = g_strdup(version);
}

const gchar *
gtkntf_theme_info_get_version(GtkNtfThemeInfo *info) {
	g_return_val_if_fail(info, NULL);

	return info->version;
}

void
gtkntf_theme_info_set_summary(GtkNtfThemeInfo *info, const gchar *summary) {
	g_return_if_fail(info);
	g_return_if_fail(summary);

	if(info->summary)
		g_free(info->summary);

	info->summary = g_strdup(summary);
}

const gchar *
gtkntf_theme_info_get_summary(GtkNtfThemeInfo *info) {
	g_return_val_if_fail(info, NULL);

	return info->summary;
}

void
gtkntf_theme_info_set_description(GtkNtfThemeInfo *info, const gchar *description) {
	g_return_if_fail(info);
	g_return_if_fail(description);

	if(info->description)
		g_free(info->description);

	info->description = g_strdup(description);
}

const gchar *
gtkntf_theme_info_get_description(GtkNtfThemeInfo *info) {
	g_return_val_if_fail(info, NULL);

	return info->description;
}

void
gtkntf_theme_info_set_author(GtkNtfThemeInfo *info, const gchar *author) {
	g_return_if_fail(info);
	g_return_if_fail(author);

	if(info->author)
		g_free(info->author);

	info->author = g_strdup(author);
}

const gchar *
gtkntf_theme_info_get_author(GtkNtfThemeInfo *info) {
	g_return_val_if_fail(info, NULL);

	return info->author;
}

void
gtkntf_theme_info_set_website(GtkNtfThemeInfo *info, const gchar *website) {
	g_return_if_fail(info);
	g_return_if_fail(website);

	if(info->website)
		g_free(info->website);

	info->website = g_strdup(website);
}

const gchar *
gtkntf_theme_info_get_website(GtkNtfThemeInfo *info) {
	g_return_val_if_fail(info, NULL);

	return info->website;
}
