#include "internal.h"

#include <gdk/gdk.h>
#include "debug.h"
#include "util.h"
#include "xmlnode.h"

#include "gtkntf_item.h"
#include "gtkntf_theme_info.h"
#include "gtkntf_theme_ops.h"
#include "gtkntf_utils.h"

#include "gtkpref_notify.h"
#include "gtkntf_theme.h"

struct _GtkNtfTheme {
	gint api_version;

	gchar *file;
	gchar *path;

	GtkNtfThemeInfo *info;
	GtkNtfThemeOptions *ops;

	GList *notifications;

	GtkNtfNotification *master;
};


static GList *probed_themes = NULL;
static GList *loaded_themes = NULL;

/***********************************************************************
 * Loading, Unloading, Probing, and Finding ...
 **********************************************************************/
GtkNtfTheme *
gtkntf_theme_new() {
	GtkNtfTheme *theme;

	theme = g_new0(GtkNtfTheme, 1);

	return theme;
}

GtkNtfTheme *
gtkntf_theme_new_from_file(const gchar *filename) {
	GtkNtfTheme *theme;
	gchar *contents;
	gint api_version;
	gsize length;
	xmlnode *root, *parent, *child;

	g_return_val_if_fail(filename, NULL);

	if(!g_file_get_contents(filename, &contents, &length, NULL)) {
		oul_debug_info("GTKNotify", "** Error: failed to get file contents\n");
		return NULL;
	}

	if(!(root = xmlnode_from_str(contents, length))) {
		oul_debug_info("GTKNotify", "** Error: Could not parse file\n");
		return NULL;
	}

	g_free(contents);

	if(!(parent = xmlnode_get_child(root, "theme"))) {
		oul_debug_info("GTKNotify", "** Error: No theme element found\n");
		xmlnode_free(root);
		return NULL;
	}

	api_version = atoi(xmlnode_get_attrib(parent, "api"));
	if(api_version != GTKNTF_THEME_API_VERSION) {
		oul_debug_info("GTKNotify", "** Error: Theme API version mismatch\n");
		xmlnode_free(root);
		return NULL;
	}

	/* allocate the theme */
	theme = gtkntf_theme_new();

	/* info to know */
	theme->api_version = api_version;
	theme->file = g_strdup(filename);
	theme->path = g_path_get_dirname(filename);

	/* get the themes info */
	if(!(child = xmlnode_get_child(parent, "info"))) {
		oul_debug_info("GTKNotify", "** Error: No info element found\n");
		gtkntf_theme_unload(theme);
		xmlnode_free(root);
		return NULL;
	}

	if(!(theme->info = gtkntf_theme_info_new_from_xmlnode(child))) {
		oul_debug_info("GTKNotify", "** Error: could not load theme info\n");
		gtkntf_theme_unload(theme);
		xmlnode_free(root);
		return NULL;
	}

	/* get the themes options */
	if(!(child = xmlnode_get_child(parent, "options"))) {
		gtkntf_theme_unload(theme);
		xmlnode_free(root);
		return NULL;
	}

	theme->ops = gtkntf_theme_options_new_from_xmlnode(child);

	/* get all the notifications */
	child = xmlnode_get_child(parent, "notification");

	while(child) {
		GtkNtfNotification *notification;
		notification = gtkntf_notification_new_from_xmlnode(theme, child);

		if(notification)
			theme->notifications = g_list_append(theme->notifications,
												 notification);

		child = xmlnode_get_next_twin(child);
	}

	/* loading was successful free the xmlnode */
	xmlnode_free(root);

	return theme;
}

GtkNtfTheme *
gtkntf_theme_find_theme_by_name(const gchar *name) {
	GtkNtfTheme *theme;
	GList *l;

	g_return_val_if_fail(name, NULL);

	for(l = loaded_themes; l; l = l->next) {
		theme = GTKNTF_THEME(l->data);

		if(!g_utf8_collate(gtkntf_theme_info_get_name(theme->info), name))
			return theme;
	}

	return NULL;
}

GtkNtfTheme *
gtkntf_theme_find_theme_by_filename(const gchar *filename) {
	GtkNtfTheme *theme;
	GList *l;

	g_return_val_if_fail(filename, NULL);

	for(l = loaded_themes; l; l = l->next) {
		theme = GTKNTF_THEME(l->data);

		if(!g_ascii_strcasecmp(gtkntf_theme_get_filename(theme), filename))
			return theme;
	}

	return NULL;
}

gchar *
gtkntf_theme_strip_name(GtkNtfTheme *theme) {
	g_return_val_if_fail(theme, NULL);

	return gtkntf_theme_info_strip_name(theme->info);
}

gboolean
gtkntf_theme_is_loaded(const gchar *filename) {
	GtkNtfTheme *theme;
	GList *l;

	g_return_val_if_fail(filename, FALSE);

	for(l = loaded_themes; l; l = l->next) {
		theme = GTKNTF_THEME(l->data);
		if(!g_ascii_strcasecmp(filename, theme->file))
			return TRUE;
	}

	return FALSE;
}

gboolean
gtkntf_theme_is_probed(const gchar *filename) {
	g_return_val_if_fail(filename, FALSE);

	if(g_list_find_custom(probed_themes, filename, gtkntf_utils_compare_strings))
		return TRUE;
	else
		return FALSE;
}

gboolean
gtkntf_theme_save_to_file(GtkNtfTheme *theme, const gchar *filename) {
	GList *l;
	gchar *api, *data;
	FILE *fp = NULL;
	xmlnode *root, *parent, *child;

	g_return_val_if_fail(theme, FALSE);
	g_return_val_if_fail(filename, FALSE);

	root = xmlnode_new("guifications");

	parent = xmlnode_new_child(root, "theme");
	api = g_strdup_printf("%d", GTKNTF_THEME_API_VERSION);
	xmlnode_set_attrib(parent, "api", api);
	g_free(api);

	if((child = gtkntf_theme_info_to_xmlnode(theme->info)))
		xmlnode_insert_child(parent, child);

	if((child = gtkntf_theme_options_to_xmlnode(theme->ops)))
		xmlnode_insert_child(parent, child);

	for(l = theme->notifications; l; l = l->next) {
		if((child = gtkntf_notification_to_xmlnode(GTKNTF_NOTIFICATION(l->data))))
			xmlnode_insert_child(parent, child);
	}

	data = xmlnode_to_formatted_str(root, NULL);

	fp = g_fopen(filename, "wb");
	if(!fp) {
		oul_debug_info("guifications", "Error trying to save theme %s\n", filename);
	} else {
		if(data)
			fprintf(fp, "%s", data);
		fclose(fp);
	}

	g_free(data);
	xmlnode_free(root);

	return TRUE;
}

void
gtkntf_theme_destory(GtkNtfTheme *theme) {
	GList *l;

	g_return_if_fail(theme);

	theme->api_version = 0;

	if(theme->file)
		g_free(theme->file);

	if(theme->path)
		g_free(theme->path);

	if(theme->info)
		gtkntf_theme_info_destroy(theme->info);

	if(theme->ops)
		gtkntf_theme_options_destroy(theme->ops);

	for(l = theme->notifications; l; l = l->next)
		gtkntf_notification_destroy(GTKNTF_NOTIFICATION(l->data));

	g_list_free(theme->notifications);
	theme->notifications = NULL;

	g_free(theme);
	theme = NULL;
}

void
gtkntf_theme_load(const gchar *filename) {
	GtkNtfTheme *theme = NULL;

	if((theme = gtkntf_theme_new_from_file(filename)))
		loaded_themes = g_list_append(loaded_themes, theme);
}

void
gtkntf_theme_unload(GtkNtfTheme *theme) {
	g_return_if_fail(theme);

	loaded_themes = g_list_remove(loaded_themes, theme);

	gtkntf_theme_destory(theme);
}

void
gtkntf_theme_probe(const gchar *filename) {
	GtkNtfTheme *theme;
	gboolean loaded;

	g_return_if_fail(filename);

	loaded = gtkntf_theme_is_loaded(filename);

	if(gtkntf_theme_is_probed(filename))		
		gtkntf_theme_unprobe(filename);

	if(loaded)
		gtkntf_theme_unload(gtkntf_theme_find_theme_by_filename(filename));

	theme = gtkntf_theme_new_from_file(filename);
	if(theme) {
		probed_themes = g_list_append(probed_themes, g_strdup(filename));

		if(loaded)
			loaded_themes = g_list_append(loaded_themes, theme);
		else
			gtkntf_theme_destory(theme);
	}
}

void
gtkntf_themes_probe() {
	GDir *dir;
	gchar *path = NULL, *probe_dirs[3];
	const gchar *file;
	gint i;

	probe_dirs[0] = g_build_filename(PKGDATADIR, "themes", NULL);
	probe_dirs[1] = g_build_filename(oul_user_dir(), "themes", NULL);
	probe_dirs[2] = NULL;

	for(i = 0; probe_dirs[i]; i++) {
		dir = g_dir_open(probe_dirs[i], 0, NULL);

		if(dir) {
			while((file = g_dir_read_name(dir))) {
				/* disallow themes in hidden dirs */
				if(file[0] == '.')
					continue;

				path = g_build_filename(probe_dirs[i], file, "theme.xml", NULL);
				if(path) {
					if(g_file_test(path, G_FILE_TEST_EXISTS)) {
						oul_debug_info("GTK Notify", "Probing %s\n", path);
						gtkntf_theme_probe(path);
					}

					g_free(path);
				}
			}

			g_dir_close(dir);
		} else if(i == 1) {
			/* if the user theme dir doesn't exist, create it */
			oul_build_dir(probe_dirs[i], S_IRUSR | S_IWUSR | S_IXUSR);
		}

		g_free(probe_dirs[i]);
	}
}

void
gtkntf_theme_unprobe(const gchar *filename) {
	GList *l, *ll;
	gchar *file;

	g_return_if_fail(filename);

	for(l = probed_themes; l; l = ll) {
		ll = l->next;

		file = (gchar*)l->data;
		if(!g_ascii_strcasecmp(file, filename)) {
			probed_themes = g_list_remove(probed_themes, file);
			g_free(file);
		}
	}
}

void
gtkntf_themes_unprobe() {
	GList *l;
	gchar *file;

	for(l = probed_themes; l; l = l->next) {
		if((file = (gchar*)l->data)) {
			oul_debug_info("GTKNotify", "unprobing %s\n", file);
			g_free(file);
		}
	}

	if(probed_themes)
		g_list_free(probed_themes);

	probed_themes = NULL;
}

/*******************************************************************************
 * Theme API
 ******************************************************************************/
gint
gtkntf_theme_get_api_version(GtkNtfTheme *theme) {
	g_return_val_if_fail(theme, -1);

	return theme->api_version;
}

const gchar *
gtkntf_theme_get_filename(GtkNtfTheme *theme) {
	g_return_val_if_fail(theme, NULL);

	return theme->file;
}

const gchar *
gtkntf_theme_get_path(GtkNtfTheme *theme) {
	g_return_val_if_fail(theme, NULL);

	return theme->path;
}

GtkNtfNotification *
gtkntf_theme_get_master(GtkNtfTheme *theme) {
	g_return_val_if_fail(theme, NULL);

	return theme->master;
}

void
gtkntf_theme_set_master(GtkNtfTheme *theme, GtkNtfNotification *notification) {
	g_return_if_fail(theme);
	g_return_if_fail(notification);

	theme->master = notification;
}

static void
gtkntf_theme_get_supported_func(gpointer key, gpointer val, gpointer data) {
	GString *str = data;
	gchar *type = key;
	gint value = GPOINTER_TO_INT(val);

	if(strlen(str->str) != 0)
		str = g_string_append(str, ", ");

	str = g_string_append(str, type);

	if(value > 1)
		g_string_append_printf(str, " (%d)", value);
}

gchar *
gtkntf_theme_get_supported_notifications(GtkNtfTheme *theme) {
	GtkNtfNotification *notification;
	GHashTable *table;
	GList *l;
	GString *str;
	const gchar *type;
	gchar *ret;
	gint value;
	gpointer pvalue;

	g_return_val_if_fail(theme, NULL);

	table = g_hash_table_new(g_str_hash, g_str_equal);

	for(l = theme->notifications; l; l = l->next) {
		notification = GTKNTF_NOTIFICATION(l->data);
		type = gtkntf_notification_get_type(notification);

		if(type && type[0] == '!')
			continue;

		pvalue = g_hash_table_lookup(table, type);
		if(pvalue)
			value = GPOINTER_TO_INT(pvalue) + 1;
		else
			value = 1;

		g_hash_table_replace(table, (gpointer)type, GINT_TO_POINTER(value));
	}

	str = g_string_new("");
	g_hash_table_foreach(table, gtkntf_theme_get_supported_func, str);
	g_hash_table_destroy(table);

	ret = str->str;
	g_string_free(str, FALSE);

	return ret;
}

void
gtkntf_theme_set_theme_info(GtkNtfTheme *theme, GtkNtfThemeInfo *info) {
	g_return_if_fail(theme);
	g_return_if_fail(info);

	if(theme->info)
		gtkntf_theme_info_destroy(theme->info);

	theme->info = info;
}

GtkNtfThemeInfo *
gtkntf_theme_get_theme_info(GtkNtfTheme *theme) {
	g_return_val_if_fail(theme, NULL);

	return theme->info;
}

void
gtkntf_theme_set_theme_options(GtkNtfTheme *theme, GtkNtfThemeOptions *ops) {
	g_return_if_fail(theme);
	g_return_if_fail(ops);

	if(theme->ops)
		gtkntf_theme_options_destroy(theme->ops);

	theme->ops = ops;
}

GtkNtfThemeOptions *
gtkntf_theme_get_theme_options(GtkNtfTheme *theme) {
	g_return_val_if_fail(theme, NULL);

	return theme->ops;
}

void
gtkntf_theme_add_notification(GtkNtfTheme *theme, GtkNtfNotification *notification) {
	const gchar *type = NULL;

	g_return_if_fail(theme);
	g_return_if_fail(notification);

	type = gtkntf_notification_get_type(notification);
	if(!g_utf8_collate(GTKNTF_NOTIFICATION_MASTER, type)) {
		if(theme->master) {
			const gchar *name = NULL;

			name = gtkntf_theme_info_get_name(theme->info);
			oul_debug_info("GTKNotify",
							"Theme %s already has a master notification\n",
							name ? name : "(NULL)");
			return;
		} else {
			theme->master = notification;
		}
	}

	theme->notifications = g_list_append(theme->notifications, notification);
}

void
gtkntf_theme_remove_notification(GtkNtfTheme *theme, GtkNtfNotification *notification) {
	const gchar *type = NULL;

	g_return_if_fail(theme);
	g_return_if_fail(notification);

	type = gtkntf_notification_get_type(notification);
	if(!g_utf8_collate(GTKNTF_NOTIFICATION_MASTER, type)) {
		oul_debug_info("GTKNotify",
						"Master notifications can not be removed\n");
		return;
	}

	theme->notifications = g_list_remove(theme->notifications, notification);
}

GList *
gtkntf_theme_get_notifications(GtkNtfTheme *theme) {
	g_return_val_if_fail(theme, NULL);

	return theme->notifications;
}

GList *
gtkntf_themes_get_all() {
	return probed_themes;
}

GList *
gtkntf_themes_get_loaded() {
	return loaded_themes;
}

void
gtkntf_themes_unload() {
	GtkNtfTheme *theme;
	GList *l = NULL, *ll = NULL;

	for(l = loaded_themes; l; l = ll) {
		ll = l->next;

		theme = GTKNTF_THEME(l->data);

		if(theme) {
			gtkntf_theme_unload(theme);
			theme = NULL;
		}
	}

	g_list_free(loaded_themes);

	loaded_themes = NULL;
}

void
gtkntf_themes_save_loaded() {
	GtkNtfTheme *theme;
	GList *l = NULL, *s = NULL;

	for(l = loaded_themes; l; l = l->next) {
		theme = GTKNTF_THEME(l->data);

		if(theme)
			s = g_list_append(s, theme->file);
	}

	oul_prefs_set_string_list(BEASY_PREFS_NTF_LOADED_THEMES, s);

	g_list_free(s);
}

void
gtkntf_themes_load_saved(){
	GList *s = NULL;
	gchar *filename = NULL;

	for(s = oul_prefs_get_string_list(BEASY_PREFS_NTF_LOADED_THEMES); s; s = s->next) {
		filename = (gchar*)s->data;

		oul_debug_info("gtkntf_themes_load_saved", "filename:%s\n", filename);
		if(gtkntf_theme_is_probed(filename)){
			oul_debug_info("gtkntf_themes_load_saved", "was loaded.\n");
			gtkntf_theme_load(filename);
		}
	}
}
