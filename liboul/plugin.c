#include "internal.h"

#include "core.h"
#include "notify.h"
#include "version.h"
#include "plugin.h"
#include "core.h"
#include "debug.h"
#include "prefs.h"
#include "util.h"
#include "signals.h"
#include "sigdef.h"

typedef struct
{
	GHashTable *commands;
	size_t command_count;

} OulPluginIpcInfo;

typedef struct
{
	OulCallback func;
	OulSignalMarshalFunc marshal;

	int num_params;
	OulValue **params;
	OulValue *ret_value;

} OulPluginIpcCommand;

static int handle;

static GList *search_paths     = NULL;
static GList *plugins          = NULL;
static GList *loaded_plugins   = NULL;
static GList *protocol_plugins = NULL;

static GList *load_queue       = NULL;
static GList *plugin_loaders   = NULL;
static GList *plugins_to_disable = NULL;

static void (*probe_cb)(void *) = NULL;
static void *probe_cb_data = NULL;
static void (*load_cb)(OulPlugin *, void *) = NULL;
static void *load_cb_data = NULL;
static void (*unload_cb)(OulPlugin *, void *) = NULL;
static void *unload_cb_data = NULL;

static gboolean
has_file_extension(const char *filename, const char *ext)
{
	int len, extlen;

	if (filename == NULL || *filename == '\0' || ext == NULL)
		return 0;

	extlen = strlen(ext);
	len = strlen(filename) - extlen;

	if (len < 0)
		return 0;

	return (strncmp(filename + len, ext, extlen) == 0);
}

static gboolean
is_native(const char *filename)
{
	const char *last_period;

	last_period = strrchr(filename, '.');
	if (last_period == NULL)
		return FALSE;

	return !(strcmp(last_period, ".dll") &
			 strcmp(last_period, ".sl") &
			 strcmp(last_period, ".so"));
}

static char *
oul_plugin_get_basename(const char *filename)
{
	const char *basename;
	const char *last_period;

	basename = strrchr(filename, G_DIR_SEPARATOR);
	if (basename != NULL)
		basename++;
	else
		basename = filename;

	if (is_native(basename) &&
		((last_period = strrchr(basename, '.')) != NULL))
			return g_strndup(basename, (last_period - basename));

	return g_strdup(basename);
}

static gboolean
loader_supports_file(OulPlugin *loader, const char *filename)
{
	GList *exts;

	for (exts = OUL_PLUGIN_LOADER_INFO(loader)->exts; exts != NULL; exts = exts->next) {
		if (has_file_extension(filename, (char *)exts->data)) {
			return TRUE;
		}
	}

	return FALSE;
}

static OulPlugin *
find_loader_for_plugin(const OulPlugin *plugin)
{
	OulPlugin *loader;
	GList *l;

	if (plugin->path == NULL)
		return NULL;

	for (l = oul_plugins_get_loaded(); l != NULL; l = l->next) {
		loader = l->data;

		if (loader->info->type == OUL_PLUGIN_LOADER &&
			loader_supports_file(loader, plugin->path)) {

			return loader;
		}

		loader = NULL;
	}

	return NULL;
}

OulPlugin *
oul_plugin_new(gboolean native, const char *path)
{
	OulPlugin *plugin;

	plugin = g_new0(OulPlugin, 1);

	plugin->native_plugin = native;
	plugin->path = g_strdup(path);

	//OUL_DBUS_REGISTER_POINTER(plugin, OulPlugin);

	return plugin;
}

OulPlugin *
oul_plugin_probe(const char *filename)
{
	OulPlugin *plugin = NULL;
	OulPlugin *loader;
	gpointer unpunned;
	gchar *basename = NULL;
	gboolean (*oul_init_plugin)(OulPlugin *);

	oul_debug_misc("plugins", "probing %s\n", filename);
	g_return_val_if_fail(filename != NULL, NULL);

	if (!g_file_test(filename, G_FILE_TEST_EXISTS))
		return NULL;

	/* If this plugin has already been probed then exit */
	basename = oul_plugin_get_basename(filename);
	plugin = oul_plugins_find_with_basename(basename);
	g_free(basename);
	if (plugin != NULL)
	{
		if (!strcmp(filename, plugin->path))
			return plugin;
		else if (!oul_plugin_is_unloadable(plugin))
		{
			oul_debug_info("plugins", "Not loading %s. "
							"Another plugin with the same name (%s) has already been loaded.\n",
							filename, plugin->path);
			return plugin;
		}
		else
		{
			/* The old plugin was a different file and it was unloadable.
			 * There's no guarantee that this new file with the same name
			 * will be loadable, but unless it fails in one of the silent
			 * ways and the first one didn't, it's not any worse.  The user
			 * will still see a greyed-out plugin, which is what we want. */
			oul_plugin_destroy(plugin);
		}
	}

	plugin = oul_plugin_new(has_file_extension(filename, G_MODULE_SUFFIX), filename);

	if (plugin->native_plugin) {
		const char *error;

		/*
		 * We pass G_MODULE_BIND_LOCAL here to prevent symbols from
		 * plugins being added to the global name space.
		 *
		 * G_MODULE_BIND_LOCAL was added in glib 2.3.3.
		 */
#if GLIB_CHECK_VERSION(2,3,3)
		plugin->handle = g_module_open(filename, G_MODULE_BIND_LOCAL);
#else
		plugin->handle = g_module_open(filename, 0);
#endif

		if (plugin->handle == NULL)
		{
			const char *error = g_module_error();
			if (error != NULL && oul_str_has_prefix(error, filename))
			{
				error = error + strlen(filename);

				/* These are just so we don't crash.  If we
				 * got this far, they should always be true. */
				if (*error == ':')
					error++;
				if (*error == ' ')
					error++;
			}

			if (error == NULL || !*error)
			{
				plugin->error = g_strdup(_("Unknown error"));
				oul_debug_error("plugins", "%s is not loadable: Unknown error\n",
						 plugin->path);
			}
			else
			{
				plugin->error = g_strdup(error);
				oul_debug_error("plugins", "%s is not loadable: %s\n",
						 plugin->path, plugin->error);
			}
#if GLIB_CHECK_VERSION(2,3,3)
			plugin->handle = g_module_open(filename, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
#else
			plugin->handle = g_module_open(filename, G_MODULE_BIND_LAZY);
#endif

			if (plugin->handle == NULL)
			{
				oul_plugin_destroy(plugin);
				return NULL;
			}
			else
			{
				/* We were able to load the plugin with lazy symbol binding.
				 * This means we're missing some symbol.  Mark it as
				 * unloadable and keep going so we get the info to display
				 * to the user so they know to rebuild this plugin. */
				plugin->unloadable = TRUE;
			}
		}

		if (!g_module_symbol(plugin->handle, "oul_init_plugin",
							 &unpunned))
		{
			oul_debug_error("plugins", "%s is not usable because the "
							 "'oul_init_plugin' symbol could not be "
							 "found.  Does the plugin call the "
							 "OUL_INIT_PLUGIN() macro?\n", plugin->path);

			g_module_close(plugin->handle);
			error = g_module_error();
			if (error != NULL)
				oul_debug_error("plugins", "Error closing module %s: %s\n",
								 plugin->path, error);
			plugin->handle = NULL;

			oul_plugin_destroy(plugin);
			return NULL;
		}
		oul_init_plugin = unpunned;

	}
	else {
		loader = find_loader_for_plugin(plugin);

		if (loader == NULL) {
			oul_plugin_destroy(plugin);
			return NULL;
		}

		oul_init_plugin = OUL_PLUGIN_LOADER_INFO(loader)->probe;
	}

	if (!oul_init_plugin(plugin) || plugin->info == NULL)
	{
		oul_plugin_destroy(plugin);
		return NULL;
	}
	else if (plugin->info->ui_requirement &&
			strcmp(plugin->info->ui_requirement, oul_core_get_ui()))
	{
		plugin->error = g_strdup_printf(_("You are using %s, but this plugin requires %s."),
					oul_core_get_ui(), plugin->info->ui_requirement);
		
		oul_debug_error("plugins", "%s is not loadable: The UI requirement is not met.(%s)\n", 
									plugin->path, plugin->error);
		
		plugin->unloadable = TRUE;
		return plugin;
	}

	/*
	 * Check to make sure a plugin has defined an id.
	 * Not having this check caused oul_plugin_unload to
	 * enter an infinite loop in certain situations by passing
	 * oul_find_plugin_by_id a NULL value. -- ecoffey
	 */
	if (plugin->info->id == NULL || *plugin->info->id == '\0')
	{
		plugin->error = g_strdup_printf(_("This plugin has not defined an ID."));
		oul_debug_error("plugins", "%s is not loadable: info->id is not defined.\n", plugin->path);
		plugin->unloadable = TRUE;
		return plugin;
	}

	/* Really old plugins. */
	if (plugin->info->magic != OUL_PLUGIN_MAGIC)
	{
		if (plugin->info->magic >= 2 && plugin->info->magic <= 4)
		{
			struct _OulPluginInfo2
			{
				unsigned int api_version;
				OulPluginType type;
				char *ui_requirement;
				unsigned long flags;
				GList *dependencies;
				OulPluginPriority priority;

				char *id;
				char *name;
				char *version;
				char *summary;
				char *description;
				char *author;
				char *homepage;

				gboolean (*load)(OulPlugin *plugin);
				gboolean (*unload)(OulPlugin *plugin);
				void (*destroy)(OulPlugin *plugin);

				void *ui_info;
				void *extra_info;
				OulPluginUiInfo *prefs_info;
				GList *(*actions)(OulPlugin *plugin, gpointer context);
			} *info2 = (struct _OulPluginInfo2 *)plugin->info;

			/* This leaks... but only for ancient plugins, so deal with it. */
			plugin->info = g_new0(OulPluginInfo, 1);

			/* We don't really need all these to display the plugin info, but
			 * I'm copying them all for good measure. */
			plugin->info->magic          = info2->api_version;
			plugin->info->type           = info2->type;
			plugin->info->ui_requirement = info2->ui_requirement;
			plugin->info->flags          = info2->flags;
			plugin->info->dependencies   = info2->dependencies;
			plugin->info->id             = info2->id;
			plugin->info->name           = info2->name;
			plugin->info->version        = info2->version;
			plugin->info->summary        = info2->summary;
			plugin->info->description    = info2->description;
			plugin->info->author         = info2->author;
			plugin->info->homepage       = info2->homepage;
			plugin->info->load           = info2->load;
			plugin->info->unload         = info2->unload;
			plugin->info->destroy        = info2->destroy;
			plugin->info->ui_info        = info2->ui_info;
			plugin->info->extra_info     = info2->extra_info;

			if (info2->api_version >= 3)
				plugin->info->prefs_info = info2->prefs_info;

			if (info2->api_version >= 4)
				plugin->info->actions    = info2->actions;


			plugin->error = g_strdup_printf(_("Plugin magic mismatch %d (need %d)"),
							 plugin->info->magic, OUL_PLUGIN_MAGIC);
			oul_debug_error("plugins", "%s is not loadable: Plugin magic mismatch %d (need %d)\n",
					  plugin->path, plugin->info->magic, OUL_PLUGIN_MAGIC);
			plugin->unloadable = TRUE;
			return plugin;
		}

		oul_debug_error("plugins", "%s is not loadable: Plugin magic mismatch %d (need %d)\n",
				 plugin->path, plugin->info->magic, OUL_PLUGIN_MAGIC);
		oul_plugin_destroy(plugin);
		return NULL;
	}

	if (plugin->info->major_version != OUL_MAJOR_VERSION ||
			plugin->info->minor_version > OUL_MINOR_VERSION)
	{
		plugin->error = g_strdup_printf(_("ABI version mismatch %d.%d.x (need %d.%d.x)"),
						 plugin->info->major_version, plugin->info->minor_version,
						 OUL_MAJOR_VERSION, OUL_MINOR_VERSION);
		oul_debug_error("plugins", "%s is not loadable: ABI version mismatch %d.%d.x (need %d.%d.x)\n",
				 plugin->path, plugin->info->major_version, plugin->info->minor_version,
				 OUL_MAJOR_VERSION, OUL_MINOR_VERSION);
		plugin->unloadable = TRUE;
		return plugin;
	}

	return plugin;
}

static gint
compare_plugins(gconstpointer a, gconstpointer b)
{
	const OulPlugin *plugina = a;
	const OulPlugin *pluginb = b;

	return strcmp(plugina->info->name, pluginb->info->name);
}

gboolean
oul_plugin_load(OulPlugin *plugin)
{
	GList *dep_list = NULL;
	GList *l;

	g_return_val_if_fail(plugin != NULL, FALSE);

	if (oul_plugin_is_loaded(plugin))
		return TRUE;

	if (oul_plugin_is_unloadable(plugin))
		return FALSE;

	g_return_val_if_fail(plugin->error == NULL, FALSE);

	/*
	 * Go through the list of the plugin's dependencies.
	 *
	 * First pass: Make sure all the plugins needed are probed.
	 */
	for (l = plugin->info->dependencies; l != NULL; l = l->next)
	{
		const char *dep_name = (const char *)l->data;
		OulPlugin *dep_plugin;

		dep_plugin = oul_plugins_find_with_id(dep_name);

		if (dep_plugin == NULL)
		{
			char *tmp;

			tmp = g_strdup_printf(_("The required plugin %s was not found. "
			                        "Please install this plugin and try again."),
			                      dep_name);

			oul_notify_error(NULL, NULL,
			                  _("Unable to load the plugin"), tmp);
			g_free(tmp);

			g_list_free(dep_list);

			return FALSE;
		}

		dep_list = g_list_append(dep_list, dep_plugin);
	}

	/* Second pass: load all the required plugins. */
	for (l = dep_list; l != NULL; l = l->next)
	{
		OulPlugin *dep_plugin = (OulPlugin *)l->data;

		if (!oul_plugin_is_loaded(dep_plugin))
		{
			if (!oul_plugin_load(dep_plugin))
			{
				char *tmp;

				tmp = g_strdup_printf(_("The required plugin %s was unable to load."),
				                      plugin->info->name);

				oul_notify_error(NULL, NULL,
				                 _("Unable to load your plugin."), tmp);
				g_free(tmp);

				g_list_free(dep_list);

				return FALSE;
			}
		}
	}

	/* Third pass: note that other plugins are dependencies of this plugin.
	 * This is done separately in case we had to bail out earlier. */
	for (l = dep_list; l != NULL; l = l->next)
	{
		OulPlugin *dep_plugin = (OulPlugin *)l->data;
		dep_plugin->dependent_plugins = g_list_prepend(dep_plugin->dependent_plugins, plugin->info->id);
	}

	g_list_free(dep_list);

	if (plugin->native_plugin)
	{
		if (plugin->info != NULL && plugin->info->load != NULL)
		{
			if (!plugin->info->load(plugin))
				return FALSE;
		}
	}
	else {
		OulPlugin *loader;
		OulPluginLoaderInfo *loader_info;

		loader = find_loader_for_plugin(plugin);

		if (loader == NULL)
			return FALSE;

		loader_info = OUL_PLUGIN_LOADER_INFO(loader);

		if (loader_info->load != NULL)
		{
			if (!loader_info->load(plugin))
				return FALSE;
		}
	}

	loaded_plugins = g_list_insert_sorted(loaded_plugins, plugin, compare_plugins);

	plugin->loaded = TRUE;

	if (load_cb != NULL)
		load_cb(plugin, load_cb_data);

	oul_signal_emit(oul_plugins_get_handle(), "plugin-load", plugin);

	return TRUE;

}

gboolean
oul_plugin_unload(OulPlugin *plugin)
{
	GList *l;
	GList *ll;

	g_return_val_if_fail(plugin != NULL, FALSE);
	g_return_val_if_fail(oul_plugin_is_loaded(plugin), FALSE);

	oul_debug_info("plugins", "Unloading plugin %s\n", plugin->info->name);

	/* Unload all plugins that depend on this plugin. */
	for (l = plugin->dependent_plugins; l != NULL; l = ll) {
		const char * dep_name = (const char *)l->data;
		OulPlugin *dep_plugin;

		/* Store a pointer to the next element in the list.
		 * This is because we'll be modifying this list in the loop. */
		ll = l->next;

		dep_plugin = oul_plugins_find_with_id(dep_name);

		if (dep_plugin != NULL && oul_plugin_is_loaded(dep_plugin))
		{
			if (!oul_plugin_unload(dep_plugin))
			{
				g_free(plugin->error);
				plugin->error = g_strdup_printf(_("%s requires %s, but it failed to unload."),
				                                _(plugin->info->name),
				                                _(dep_plugin->info->name));
				return FALSE;
			}
			else
			{
#if 0
				/* This isn't necessary. This has already been done when unloading dep_plugin. */
				plugin->dependent_plugins = g_list_delete_link(plugin->dependent_plugins, l);
#endif
			}
		}
	}

	/* Remove this plugin from each dependency's dependent_plugins list. */
	for (l = plugin->info->dependencies; l != NULL; l = l->next)
	{
		const char *dep_name = (const char *)l->data;
		OulPlugin *dependency;

		dependency = oul_plugins_find_with_id(dep_name);

		if (dependency != NULL)
			dependency->dependent_plugins = g_list_remove(dependency->dependent_plugins, plugin->info->id);
		else
			oul_debug_error("plugins", "Unable to remove from dependency list for %s\n", dep_name);
	}

	if (plugin->native_plugin) {
		if (plugin->info->unload && !plugin->info->unload(plugin))
			return FALSE;
        
        /**
		if (plugin->info->type == OUL_PLUGIN_PROTOCOL) {
			OulPluginProtocolInfo *prpl_info;
			GList *l;

			prpl_info = OUL_PLUGIN_PROTOCOL_INFO(plugin);

			for (l = prpl_info->user_splits; l != NULL; l = l->next)
				oul_account_user_split_destroy(l->data);

			for (l = prpl_info->protocol_options; l != NULL; l = l->next)
				oul_account_option_destroy(l->data);

			if (prpl_info->user_splits != NULL) {
				g_list_free(prpl_info->user_splits);
				prpl_info->user_splits = NULL;
			}

			if (prpl_info->protocol_options != NULL) {
				g_list_free(prpl_info->protocol_options);
				prpl_info->protocol_options = NULL;
			}
		}
        **/
	} else {
		OulPlugin *loader;
		OulPluginLoaderInfo *loader_info;

		loader = find_loader_for_plugin(plugin);

		if (loader == NULL)
			return FALSE;

		loader_info = OUL_PLUGIN_LOADER_INFO(loader);

		if (loader_info->unload && !loader_info->unload(plugin))
			return FALSE;
	}

	/* cancel any pending dialogs the plugin has */
	//oul_request_close_with_handle(plugin);
	oul_notify_close_with_handle(plugin);

	oul_signals_disconnect_by_handle(plugin);
	oul_plugin_ipc_unregister_all(plugin);

	loaded_plugins = g_list_remove(loaded_plugins, plugin);
	plugins_to_disable = g_list_remove(plugins_to_disable, plugin);
	plugin->loaded = FALSE;

	/* We wouldn't be anywhere near here if the plugin wasn't loaded, so
	 * if plugin->error is set at all, it had to be from a previous
	 * unload failure.  It's obviously okay now.
	 */
	g_free(plugin->error);
	plugin->error = NULL;

	if (unload_cb != NULL)
		unload_cb(plugin, unload_cb_data);

	oul_signal_emit(oul_plugins_get_handle(), "plugin-unload", plugin);

	oul_prefs_disconnect_by_handle(plugin);

	return TRUE;
}

void
oul_plugin_disable(OulPlugin *plugin)
{
	g_return_if_fail(plugin != NULL);

	if (!g_list_find(plugins_to_disable, plugin))
		plugins_to_disable = g_list_prepend(plugins_to_disable, plugin);
}

gboolean
oul_plugin_reload(OulPlugin *plugin)
{
	g_return_val_if_fail(plugin != NULL, FALSE);
	g_return_val_if_fail(oul_plugin_is_loaded(plugin), FALSE);

	if (!oul_plugin_unload(plugin))
		return FALSE;

	if (!oul_plugin_load(plugin))
		return FALSE;

	return TRUE;
}

void
oul_plugin_destroy(OulPlugin *plugin)
{
	g_return_if_fail(plugin != NULL);

	if (oul_plugin_is_loaded(plugin))
		oul_plugin_unload(plugin);

	plugins = g_list_remove(plugins, plugin);

	if (load_queue != NULL)
		load_queue = g_list_remove(load_queue, plugin);

	/* true, this may leak a little memory if there is a major version
	 * mismatch, but it's a lot better than trying to free something
	 * we shouldn't, and crashing while trying to load an old plugin */
	if(plugin->info == NULL || plugin->info->magic != OUL_PLUGIN_MAGIC ||
			plugin->info->major_version != OUL_MAJOR_VERSION)
	{
		if(plugin->handle)
			g_module_close(plugin->handle);

		g_free(plugin->path);
		g_free(plugin->error);

		//OUL_DBUS_UNREGISTER_POINTER(plugin);

		g_free(plugin);
		return;
	}

	if (plugin->info != NULL)
		g_list_free(plugin->info->dependencies);

	if (plugin->native_plugin)
	{
		if (plugin->info != NULL && plugin->info->type == OUL_PLUGIN_LOADER)
		{
			OulPluginLoaderInfo *loader_info;
			GList *exts, *l, *next_l;
			OulPlugin *p2;

			loader_info = OUL_PLUGIN_LOADER_INFO(plugin);

			if (loader_info != NULL && loader_info->exts != NULL)
			{
				for (exts = OUL_PLUGIN_LOADER_INFO(plugin)->exts;
					 exts != NULL;
					 exts = exts->next) {

					for (l = oul_plugins_get_all(); l != NULL; l = next_l)
					{
						next_l = l->next;

						p2 = l->data;

						if (p2->path != NULL &&
							has_file_extension(p2->path, exts->data))
						{
							oul_plugin_destroy(p2);
						}
					}
				}

				g_list_free(loader_info->exts);
			}

			plugin_loaders = g_list_remove(plugin_loaders, plugin);
		}

		if (plugin->info != NULL && plugin->info->destroy != NULL)
			plugin->info->destroy(plugin);

		if (plugin->handle != NULL)
			g_module_close(plugin->handle);
	}
	else
	{
		OulPlugin *loader;
		OulPluginLoaderInfo *loader_info;

		loader = find_loader_for_plugin(plugin);

		if (loader != NULL)
		{
			loader_info = OUL_PLUGIN_LOADER_INFO(loader);

			if (loader_info->destroy != NULL)
				loader_info->destroy(plugin);
		}
	}

	g_free(plugin->path);
	g_free(plugin->error);

	//OUL_DBUS_UNREGISTER_POINTER(plugin);

	g_free(plugin);
}

gboolean
oul_plugin_is_loaded(const OulPlugin *plugin)
{
	g_return_val_if_fail(plugin != NULL, FALSE);

	return plugin->loaded;
}

gboolean
oul_plugin_is_unloadable(const OulPlugin *plugin)
{
	g_return_val_if_fail(plugin != NULL, FALSE);

	return plugin->unloadable;
}

const gchar *
oul_plugin_get_id(const OulPlugin *plugin) {
	g_return_val_if_fail(plugin, NULL);
	g_return_val_if_fail(plugin->info, NULL);

	return plugin->info->id;
}

const gchar *
oul_plugin_get_name(const OulPlugin *plugin) {
	g_return_val_if_fail(plugin, NULL);
	g_return_val_if_fail(plugin->info, NULL);

	return _(plugin->info->name);
}

const gchar *
oul_plugin_get_version(const OulPlugin *plugin) {
	g_return_val_if_fail(plugin, NULL);
	g_return_val_if_fail(plugin->info, NULL);

	return plugin->info->version;
}

const gchar *
oul_plugin_get_summary(const OulPlugin *plugin) {
	g_return_val_if_fail(plugin, NULL);
	g_return_val_if_fail(plugin->info, NULL);

	return _(plugin->info->summary);
}

const gchar *
oul_plugin_get_description(const OulPlugin *plugin) {
	g_return_val_if_fail(plugin, NULL);
	g_return_val_if_fail(plugin->info, NULL);

	return _(plugin->info->description);
}

const gchar *
oul_plugin_get_author(const OulPlugin *plugin) {
	g_return_val_if_fail(plugin, NULL);
	g_return_val_if_fail(plugin->info, NULL);

	return _(plugin->info->author);
}

const gchar *
oul_plugin_get_homepage(const OulPlugin *plugin) {
	g_return_val_if_fail(plugin, NULL);
	g_return_val_if_fail(plugin->info, NULL);

	return plugin->info->homepage;
}

/**************************************************************************
 * Plugin IPC
 **************************************************************************/
static void
destroy_ipc_info(void *data)
{
	OulPluginIpcCommand *ipc_command = (OulPluginIpcCommand *)data;
	int i;

	if (ipc_command->params != NULL)
	{
		for (i = 0; i < ipc_command->num_params; i++)
			oul_value_destroy(ipc_command->params[i]);

		g_free(ipc_command->params);
	}

	if (ipc_command->ret_value != NULL)
		oul_value_destroy(ipc_command->ret_value);

	g_free(ipc_command);
}

gboolean
oul_plugin_ipc_register(OulPlugin *plugin, const char *command,
						 OulCallback func, OulSignalMarshalFunc marshal,
						 OulValue *ret_value, int num_params, ...)
{
	OulPluginIpcInfo *ipc_info;
	OulPluginIpcCommand *ipc_command;

	g_return_val_if_fail(plugin  != NULL, FALSE);
	g_return_val_if_fail(command != NULL, FALSE);
	g_return_val_if_fail(func    != NULL, FALSE);
	g_return_val_if_fail(marshal != NULL, FALSE);

	if (plugin->ipc_data == NULL)
	{
		ipc_info = plugin->ipc_data = g_new0(OulPluginIpcInfo, 1);
		ipc_info->commands = g_hash_table_new_full(g_str_hash, g_str_equal,
												   g_free, destroy_ipc_info);
	}
	else
		ipc_info = (OulPluginIpcInfo *)plugin->ipc_data;

	ipc_command = g_new0(OulPluginIpcCommand, 1);
	ipc_command->func       = func;
	ipc_command->marshal    = marshal;
	ipc_command->num_params = num_params;
	ipc_command->ret_value  = ret_value;

	if (num_params > 0)
	{
		va_list args;
		int i;

		ipc_command->params = g_new0(OulValue *, num_params);

		va_start(args, num_params);

		for (i = 0; i < num_params; i++)
			ipc_command->params[i] = va_arg(args, OulValue *);

		va_end(args);
	}

	g_hash_table_replace(ipc_info->commands, g_strdup(command), ipc_command);

	ipc_info->command_count++;

	return TRUE;
}

void
oul_plugin_ipc_unregister(OulPlugin *plugin, const char *command)
{
	OulPluginIpcInfo *ipc_info;

	g_return_if_fail(plugin  != NULL);
	g_return_if_fail(command != NULL);

	ipc_info = (OulPluginIpcInfo *)plugin->ipc_data;

	if (ipc_info == NULL ||
		g_hash_table_lookup(ipc_info->commands, command) == NULL)
	{
		oul_debug_error("plugins",
						 "IPC command '%s' was not registered for plugin %s\n",
						 command, plugin->info->name);
		return;
	}

	g_hash_table_remove(ipc_info->commands, command);

	ipc_info->command_count--;

	if (ipc_info->command_count == 0)
	{
		g_hash_table_destroy(ipc_info->commands);
		g_free(ipc_info);

		plugin->ipc_data = NULL;
	}
}

void
oul_plugin_ipc_unregister_all(OulPlugin *plugin)
{
	OulPluginIpcInfo *ipc_info;

	g_return_if_fail(plugin != NULL);

	if (plugin->ipc_data == NULL)
		return; /* Silently ignore it. */

	ipc_info = (OulPluginIpcInfo *)plugin->ipc_data;

	g_hash_table_destroy(ipc_info->commands);
	g_free(ipc_info);

	plugin->ipc_data = NULL;
}

gboolean
oul_plugin_ipc_get_params(OulPlugin *plugin, const char *command,
						   OulValue **ret_value, int *num_params,
						   OulValue ***params)
{
	OulPluginIpcInfo *ipc_info;
	OulPluginIpcCommand *ipc_command;

	g_return_val_if_fail(plugin  != NULL, FALSE);
	g_return_val_if_fail(command != NULL, FALSE);

	ipc_info = (OulPluginIpcInfo *)plugin->ipc_data;

	if (ipc_info == NULL ||
		(ipc_command = g_hash_table_lookup(ipc_info->commands,
										   command)) == NULL)
	{
		oul_debug_error("plugins",
						 "IPC command '%s' was not registered for plugin %s\n",
						 command, plugin->info->name);

		return FALSE;
	}

	if (num_params != NULL)
		*num_params = ipc_command->num_params;

	if (params != NULL)
		*params = ipc_command->params;

	if (ret_value != NULL)
		*ret_value = ipc_command->ret_value;

	return TRUE;
}

void *
oul_plugin_ipc_call(OulPlugin *plugin, const char *command,
					 gboolean *ok, ...)
{
	OulPluginIpcInfo *ipc_info;
	OulPluginIpcCommand *ipc_command;
	va_list args;
	void *ret_value;

	if (ok != NULL)
		*ok = FALSE;

	g_return_val_if_fail(plugin  != NULL, NULL);
	g_return_val_if_fail(command != NULL, NULL);

	ipc_info = (OulPluginIpcInfo *)plugin->ipc_data;

	if (ipc_info == NULL ||
		(ipc_command = g_hash_table_lookup(ipc_info->commands,
										   command)) == NULL)
	{
		oul_debug_error("plugins",
						 "IPC command '%s' was not registered for plugin %s\n",
						 command, plugin->info->name);

		return NULL;
	}

	va_start(args, ok);
	ipc_command->marshal(ipc_command->func, args, NULL, &ret_value);
	va_end(args);

	if (ok != NULL)
		*ok = TRUE;

	return ret_value;
}

/**************************************************************************
 * Plugins subsystem
 **************************************************************************/
void *
oul_plugins_get_handle(void) {

	return &handle;
}

void
oul_plugins_init(void)
{
    oul_debug_info("plugins", "plugins sub system init...\n");

	void *handle = oul_plugins_get_handle();

	oul_prefs_add_none("/plugins");
	oul_prefs_add_none("/plugins/core");
	oul_prefs_add_none("/plugins/gtk");
   
    /* add search path 
         * 1. ~/.oul/plugins
         * 2. /usr/lib/beasy/
	*/
	gchar *search_path = g_build_filename(oul_user_dir(), "plugins", NULL);
	oul_plugins_add_search_path(search_path);
	g_free(search_path);

	oul_plugins_add_search_path(LIBDIR);

    /* register plugin-load and plugin-unload signals */
	oul_signal_register(handle, "plugin-load",
						 oul_marshal_VOID__POINTER,
						 NULL, 1,
						 oul_value_new(OUL_TYPE_SUBTYPE,
										OUL_SUBTYPE_PLUGIN));
	
	oul_signal_register(handle, "plugin-unload",
						 oul_marshal_VOID__POINTER,
						 NULL, 1,
						 oul_value_new(OUL_TYPE_SUBTYPE, OUL_SUBTYPE_PLUGIN));


}

void
oul_plugins_uninit(void)
{
	void *handle = oul_plugins_get_handle();

	oul_signals_disconnect_by_handle(handle);
	oul_signals_unregister_by_instance(handle);
}

/**************************************************************************
 * Plugins API
 **************************************************************************/
void
oul_plugins_add_search_path(const char *path)
{
	g_return_if_fail(path != NULL);

	if (g_list_find_custom(search_paths, path, (GCompareFunc)strcmp))
		return;

	search_paths = g_list_append(search_paths, g_strdup(path));
}

void
oul_plugins_unload_all(void)
{

	while (loaded_plugins != NULL)
		oul_plugin_unload(loaded_plugins->data);

}

void
oul_plugins_destroy_all(void)
{
	while (plugins != NULL)
		oul_plugin_destroy(plugins->data);

}

void
oul_plugins_save_loaded(const char *key)
{
	GList *pl;
	GList *files = NULL;

	for (pl = oul_plugins_get_loaded(); pl != NULL; pl = pl->next) {
		OulPlugin *plugin = pl->data;

		if (plugin->info->type != OUL_PLUGIN_LOADER &&
		    !g_list_find(plugins_to_disable, plugin)) {
			files = g_list_append(files, plugin->path);
		}
	}
	
	oul_prefs_set_path_list(key, files);
	g_list_free(files);
}

void
oul_plugins_load_saved(const char *key)
{
	GList *f, *files = NULL;

	g_return_if_fail(key != NULL);

	files = oul_prefs_get_path_list(key);

	g_return_if_fail(files != NULL);

	for (f = files; f; f = f->next)
	{
		char *filename;
		char *basename;
		OulPlugin *plugin;

		if (f->data == NULL)
			continue;

		filename = f->data;

		/*
		 * We don't know if the filename uses Windows or Unix path
		 * separators (because people might be sharing a prefs.xml
		 * file across systems), so we find the last occurrence
		 * of either.
		 */
		basename = strrchr(filename, '/');
		if ((basename == NULL) || (basename < strrchr(filename, '\\')))
			basename = strrchr(filename, '\\');
		if (basename != NULL)
			basename++;

		/* Strip the extension */
		if (basename)
			basename = oul_plugin_get_basename(basename);

		if (((plugin = oul_plugins_find_with_filename(filename)) != NULL) ||
				(basename && (plugin = oul_plugins_find_with_basename(basename)) != NULL) ||
				((plugin = oul_plugin_probe(filename)) != NULL))
		{
			oul_debug_info("plugins", "Loading saved plugin %s\n",
							plugin->path);
			oul_plugin_load(plugin);
		}
		else
		{
			oul_debug_error("plugins", "Unable to find saved plugin %s\n",
							 filename);
		}

		g_free(basename);

		g_free(f->data);
	}

	g_list_free(files);
}


void
oul_plugins_probe(const char *ext)
{
	GDir *dir;
	const gchar *file;
	gchar *path;
	OulPlugin *plugin;
	GList *cur;
	const char *search_path;

	if (!g_module_supported())
		return;

	/* Probe plugins */
	for (cur = search_paths; cur != NULL; cur = cur->next)
	{
		search_path = cur->data;

		dir = g_dir_open(search_path, 0, NULL);

		if (dir != NULL)
		{
			while ((file = g_dir_read_name(dir)) != NULL)
			{
				path = g_build_filename(search_path, file, NULL);

				if (ext == NULL || has_file_extension(file, ext))
					plugin = oul_plugin_probe(path);

				g_free(path);
			}

			g_dir_close(dir);
		}
	}

	/* See if we have any plugins waiting to load */
	while (load_queue != NULL)
	{
		plugin = (OulPlugin *)load_queue->data;

		load_queue = g_list_remove(load_queue, plugin);

		if (plugin == NULL || plugin->info == NULL)
			continue;

		if (plugin->info->type == OUL_PLUGIN_LOADER)
		{
			/* We'll just load this right now. */
			if (!oul_plugin_load(plugin))
			{
				oul_plugin_destroy(plugin);

				continue;
			}

			plugin_loaders = g_list_append(plugin_loaders, plugin);

			for (cur = OUL_PLUGIN_LOADER_INFO(plugin)->exts;
				 cur != NULL;
				 cur = cur->next)
			{
				oul_plugins_probe(cur->data);
			}
		}
	}

	if (probe_cb != NULL)
		probe_cb(probe_cb_data);

}

gboolean
oul_plugin_register(OulPlugin *plugin)
{
	g_return_val_if_fail(plugin != NULL, FALSE);

	/* If this plugin has been registered already then exit */
	if (g_list_find(plugins, plugin))
		return TRUE;

	/* Ensure the plugin has the requisite information */
	if (plugin->info->type == OUL_PLUGIN_LOADER)
	{
		OulPluginLoaderInfo *loader_info;

		loader_info = OUL_PLUGIN_LOADER_INFO(plugin);

		if (loader_info == NULL)
		{
			oul_debug_error("plugins", "%s is not loadable, loader plugin missing loader_info\n",
							   plugin->path);
			return FALSE;
		}
	}
    /**
	else if (plugin->info->type == OUL_PLUGIN_PROTOCOL)
	{
		OulPluginProtocolInfo *prpl_info;

		prpl_info = OUL_PLUGIN_PROTOCOL_INFO(plugin);

		if (prpl_info == NULL)
		{
			oul_debug_error("plugins", "%s is not loadable, protocol plugin missing prpl_info\n",
							   plugin->path);
			return FALSE;
		}
	}
    	**/

	/* This plugin should be probed and maybe loaded--add it to the queue */
	load_queue = g_list_append(load_queue, plugin);
	if (plugin->info != NULL)
	{
        /**
		if (plugin->info->type == OUL_PLUGIN_PROTOCOL)
			protocol_plugins = g_list_insert_sorted(protocol_plugins, plugin,
													(GCompareFunc)compare_prpl);
        	**/
		if (plugin->info->load != NULL)
			if (!plugin->info->load(plugin))
				return FALSE;
	}

	plugins = g_list_append(plugins, plugin);

	return TRUE;
}

gboolean
oul_plugins_enabled(void)
{
	return TRUE;
}

void
oul_plugins_register_probe_notify_cb(void (*func)(void *), void *data)
{
	probe_cb = func;
	probe_cb_data = data;
}

void
oul_plugins_unregister_probe_notify_cb(void (*func)(void *))
{
	probe_cb = NULL;
	probe_cb_data = NULL;
}

void
oul_plugins_register_load_notify_cb(void (*func)(OulPlugin *, void *),
									 void *data)
{
	load_cb = func;
	load_cb_data = data;
}

void
oul_plugins_unregister_load_notify_cb(void (*func)(OulPlugin *, void *))
{
	load_cb = NULL;
	load_cb_data = NULL;
}

void
oul_plugins_register_unload_notify_cb(void (*func)(OulPlugin *, void *),
									   void *data)
{
	unload_cb = func;
	unload_cb_data = data;
}

void
oul_plugins_unregister_unload_notify_cb(void (*func)(OulPlugin *, void *))
{
	unload_cb = NULL;
	unload_cb_data = NULL;
}

OulPlugin *
oul_plugins_find_with_name(const char *name)
{
	OulPlugin *plugin;
	GList *l;

	for (l = plugins; l != NULL; l = l->next) {
		plugin = l->data;

		if (!strcmp(plugin->info->name, name))
			return plugin;
	}

	return NULL;
}

OulPlugin *
oul_plugins_find_with_filename(const char *filename)
{
	OulPlugin *plugin;
	GList *l;

	for (l = plugins; l != NULL; l = l->next) {
		plugin = l->data;

		if (plugin->path != NULL && !strcmp(plugin->path, filename))
			return plugin;
	}

	return NULL;
}

OulPlugin *
oul_plugins_find_with_basename(const char *basename)
{
	OulPlugin *plugin;
	GList *l;
	char *tmp;

	g_return_val_if_fail(basename != NULL, NULL);

	for (l = plugins; l != NULL; l = l->next)
	{
		plugin = (OulPlugin *)l->data;

		if (plugin->path != NULL) {
			tmp = oul_plugin_get_basename(plugin->path);
			if (!strcmp(tmp, basename))
			{
				g_free(tmp);
				return plugin;
			}
			g_free(tmp);
		}
	}

	return NULL;
}

OulPlugin *
oul_plugins_find_with_id(const char *id)
{
	OulPlugin *plugin;
	GList *l;

	g_return_val_if_fail(id != NULL, NULL);

	for (l = plugins; l != NULL; l = l->next)
	{
		plugin = l->data;

		if (plugin->info->id != NULL && !strcmp(plugin->info->id, id))
			return plugin;
	}

	return NULL;
}

GList *
oul_plugins_get_loaded(void)
{
	return loaded_plugins;
}

GList *
oul_plugins_get_protocols(void)
{
	return protocol_plugins;
}

GList *
oul_plugins_get_all(void)
{
	return plugins;
}


OulPluginAction *
oul_plugin_action_new(const char* label, void (*callback)(OulPluginAction *))
{
	OulPluginAction *act = g_new0(OulPluginAction, 1);

	act->label = g_strdup(label);
	act->callback = callback;

	return act;
}

void
oul_plugin_action_free(OulPluginAction *action)
{
	g_return_if_fail(action != NULL);

	g_free(action->label);
	g_free(action);
}

