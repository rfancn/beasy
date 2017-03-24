#ifndef _OUL_PLUGIN_H_
#define _OUL_PLUGIN_H_

#include <glib/glist.h>
#include <gmodule.h>

#include "value.h"
#include "signals.h"
#include "pluginpref.h"

typedef struct _OulPlugin           OulPlugin;
typedef struct _OulPluginInfo       OulPluginInfo;
typedef struct _OulPluginUiInfo     OulPluginUiInfo;
typedef struct _OulPluginLoaderInfo OulPluginLoaderInfo;

typedef struct _OulPluginAction     OulPluginAction;

typedef int OulPluginPriority; /**< Plugin priority. */


/**
 * Plugin types.
 */
typedef enum
{
	OUL_PLUGIN_UNKNOWN  = -1,  /**< Unknown type.    */
	OUL_PLUGIN_STANDARD = 0,   /**< Standard plugin. */
	OUL_PLUGIN_LOADER,         /**< Loader plugin.   */
	OUL_PLUGIN_PROTOCOL        /**< Protocol plugin. */

} OulPluginType;

#define OUL_PRIORITY_DEFAULT     0
#define OUL_PRIORITY_HIGHEST  9999
#define OUL_PRIORITY_LOWEST  -9999

#define OUL_PLUGIN_FLAG_INVISIBLE 0x01

#define OUL_PLUGIN_MAGIC 5 /* once we hit 6.0.0 I think we can remove this */

/**
 * Detailed information about a plugin.
 *
 * This is used in the version 2.0 API and up.
 */
struct _OulPluginInfo
{
	unsigned int magic;
	unsigned int major_version;
	unsigned int minor_version;
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

	/**
	 * If a plugin defines a 'load' function, and it returns FALSE,
	 * then the plugin will not be loaded.
	 */
	gboolean (*load)(OulPlugin *plugin);
	gboolean (*unload)(OulPlugin *plugin);
	void (*destroy)(OulPlugin *plugin);

	void *ui_info; /**< Used only by UI-specific plugins to build a preference screen with a custom UI */
	void *extra_info;
	OulPluginUiInfo *prefs_info; /**< Used by any plugin to display preferences.  If #ui_info has been specified, this will be ignored. */
	GList *(*actions)(OulPlugin *plugin, gpointer context);

	void (*_oul_reserved1)(void);
	void (*_oul_reserved2)(void);
	void (*_oul_reserved3)(void);
	void (*_oul_reserved4)(void);
};

/**
 * Extra information for loader plugins.
 */
struct _OulPluginLoaderInfo
{
	GList *exts;

	gboolean (*probe)(OulPlugin *plugin);
	gboolean (*load)(OulPlugin *plugin);
	gboolean (*unload)(OulPlugin *plugin);
	void     (*destroy)(OulPlugin *plugin);

	void (*_oul_reserved1)(void);
	void (*_oul_reserved2)(void);
	void (*_oul_reserved3)(void);
	void (*_oul_reserved4)(void);
};

/**
 * A plugin handle.
 */
struct _OulPlugin
{
	gboolean native_plugin;                /**< Native C plugin.          */
	gboolean loaded;                       /**< The loaded state.         */
	void *handle;                          /**< The module handle.        */
	char *path;                            /**< The path to the plugin.   */
	OulPluginInfo *info;                  /**< The plugin information.   */
	char *error;
	void *ipc_data;                        /**< IPC data.                 */
	void *extra;                           /**< Plugin-specific data.     */
	gboolean unloadable;                   /**< Unloadable                */
	GList *dependent_plugins;              /**< Plugins depending on this */

	void (*_oul_reserved1)(void);
	void (*_oul_reserved2)(void);
	void (*_oul_reserved3)(void);
	void (*_oul_reserved4)(void);
};

#define OUL_PLUGIN_LOADER_INFO(plugin) \
	((OulPluginLoaderInfo *)(plugin)->info->extra_info)

struct _OulPluginUiInfo {
	OulPluginPrefFrame *(*get_plugin_pref_frame)(OulPlugin *plugin);

	int page_num;                                         /**< Reserved */
	OulPluginPrefFrame *frame;                           /**< Reserved */

	void (*_oul_reserved1)(void);
	void (*_oul_reserved2)(void);
	void (*_oul_reserved3)(void);
	void (*_oul_reserved4)(void);
};

#define OUL_PLUGIN_HAS_PREF_FRAME(plugin) \
	((plugin)->info != NULL && (plugin)->info->prefs_info != NULL)

#define OUL_PLUGIN_UI_INFO(plugin) \
	((OulPluginUiInfo*)(plugin)->info->prefs_info)


/**
 * The structure used in the actions member of OulPluginInfo
 */
struct _OulPluginAction {
	char *label;
	void (*callback)(OulPluginAction *);

	/** set to the owning plugin */
	OulPlugin *plugin;

	/** NULL for plugin actions menu, set to the OulConnection for
	    account actions menu */
	gpointer context;
	
	gpointer user_data;
};

#define OUL_PLUGIN_HAS_ACTIONS(plugin) \
	((plugin)->info != NULL && (plugin)->info->actions != NULL)

#define OUL_PLUGIN_ACTIONS(plugin, context) \
	(OUL_PLUGIN_HAS_ACTIONS(plugin)? \
	(plugin)->info->actions(plugin, context): NULL)


/**
 * Handles the initialization of modules.
 */
#if !defined(OUL_PLUGINS) || defined(OUL_STATIC_PRPL)
# define _FUNC_NAME(x) oul_init_##x##_plugin
# define OUL_INIT_PLUGIN(pluginname, initfunc, plugininfo) \
	gboolean _FUNC_NAME(pluginname)(void);\
	gboolean _FUNC_NAME(pluginname)(void) { \
		OulPlugin *plugin = oul_plugin_new(TRUE, NULL); \
		plugin->info = &(plugininfo); \
		initfunc((plugin)); \
		oul_plugin_load((plugin)); \
		return oul_plugin_register(plugin); \
	}
#else /* OUL_PLUGINS  && !OUL_STATIC_PRPL */
# define OUL_INIT_PLUGIN(pluginname, initfunc, plugininfo) \
	G_MODULE_EXPORT gboolean oul_init_plugin(OulPlugin *plugin); \
	G_MODULE_EXPORT gboolean oul_init_plugin(OulPlugin *plugin) { \
		plugin->info = &(plugininfo); \
		initfunc((plugin)); \
		return oul_plugin_register(plugin); \
	}
#endif


#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/** @name Plugin API                                                      */
/**************************************************************************/
/*@{*/

/**
 * Creates a new plugin structure.
 *
 * @param native Whether or not the plugin is native.
 * @param path   The path to the plugin, or @c NULL if statically compiled.
 *
 * @return A new OulPlugin structure.
 */
OulPlugin *oul_plugin_new(gboolean native, const char *path);

/**
 * Probes a plugin, retrieving the information on it and adding it to the
 * list of available plugins.
 *
 * @param filename The plugin's filename.
 *
 * @return The plugin handle.
 *
 * @see oul_plugin_load()
 * @see oul_plugin_destroy()
 */
OulPlugin *oul_plugin_probe(const char *filename);

/**
 * Registers a plugin and prepares it for loading.
 *
 * This shouldn't be called by anything but the internal module code.
 * Plugins should use the OUL_INIT_PLUGIN() macro to register themselves
 * with the core.
 *
 * @param plugin The plugin to register.
 *
 * @return @c TRUE if the plugin was registered successfully.  Otherwise
 *         @c FALSE is returned (this happens if the plugin does not contain
 *         the necessary information).
 */
gboolean oul_plugin_register(OulPlugin *plugin);

/**
 * Attempts to load a previously probed plugin.
 *
 * @param plugin The plugin to load.
 *
 * @return @c TRUE if successful, or @c FALSE otherwise.
 *
 * @see oul_plugin_reload()
 * @see oul_plugin_unload()
 */
gboolean oul_plugin_load(OulPlugin *plugin);

/**
 * Unloads the specified plugin.
 *
 * @param plugin The plugin handle.
 *
 * @return @c TRUE if successful, or @c FALSE otherwise.
 *
 * @see oul_plugin_load()
 * @see oul_plugin_reload()
 */
gboolean oul_plugin_unload(OulPlugin *plugin);

/**
 * Disable a plugin.
 *
 * This function adds the plugin to a list of plugins to "disable at the next
 * startup" by excluding said plugins from the list of plugins to save.  The
 * UI needs to call oul_plugins_save_loaded() after calling this for it
 * to have any effect.
 *
 * @since 2.3.0
 */
void oul_plugin_disable(OulPlugin *plugin);

/**
 * Reloads a plugin.
 *
 * @param plugin The old plugin handle.
 *
 * @return @c TRUE if successful, or @c FALSE otherwise.
 *
 * @see oul_plugin_load()
 * @see oul_plugin_unload()
 */
gboolean oul_plugin_reload(OulPlugin *plugin);

/**
 * Unloads a plugin and destroys the structure from memory.
 *
 * @param plugin The plugin handle.
 */
void oul_plugin_destroy(OulPlugin *plugin);

/**
 * Returns whether or not a plugin is currently loaded.
 *
 * @param plugin The plugin.
 *
 * @return @c TRUE if loaded, or @c FALSE otherwise.
 */
gboolean oul_plugin_is_loaded(const OulPlugin *plugin);

/**
 * Returns whether or not a plugin is unloadable.
 *
 * If this returns @c TRUE, the plugin is guaranteed to not
 * be loadable. However, a return value of @c FALSE does not
 * guarantee the plugin is loadable.
 *
 * @param plugin The plugin.
 *
 * @return @c TRUE if the plugin is known to be unloadable,\
 *         @c FALSE otherwise
 */
gboolean oul_plugin_is_unloadable(const OulPlugin *plugin);

/**
 * Returns a plugin's id.
 *
 * @param plugin The plugin.
 *
 * @return The plugin's id.
 */
const gchar *oul_plugin_get_id(const OulPlugin *plugin);

/**
 * Returns a plugin's name.
 *
 * @param plugin The plugin.
 * 
 * @return THe name of the plugin, or @c NULL.
 */
const gchar *oul_plugin_get_name(const OulPlugin *plugin);

/**
 * Returns a plugin's version.
 *
 * @param plugin The plugin.
 *
 * @return The plugin's version or @c NULL.
 */
const gchar *oul_plugin_get_version(const OulPlugin *plugin);

/**
 * Returns a plugin's summary.
 *
 * @param plugin The plugin.
 *
 * @return The plugin's summary.
 */
const gchar *oul_plugin_get_summary(const OulPlugin *plugin);

/**
 * Returns a plugin's description.
 *
 * @param plugin The plugin.
 *
 * @return The plugin's description.
 */
const gchar *oul_plugin_get_description(const OulPlugin *plugin);

/**
 * Returns a plugin's author.
 *
 * @param plugin The plugin.
 *
 * @return The plugin's author.
 */
const gchar *oul_plugin_get_author(const OulPlugin *plugin);

/**
 * Returns a plugin's homepage.
 *
 * @param plugin The plugin.
 *
 * @return The plugin's homepage.
 */
const gchar *oul_plugin_get_homepage(const OulPlugin *plugin);

/*@}*/

/**************************************************************************/
/** @name Plugin IPC API                                                  */
/**************************************************************************/
/*@{*/

/**
 * Registers an IPC command in a plugin.
 *
 * @param plugin     The plugin to register the command with.
 * @param command    The name of the command.
 * @param func       The function to execute.
 * @param marshal    The marshalling function.
 * @param ret_value  The return value type.
 * @param num_params The number of parameters.
 * @param ...        The parameter types.
 *
 * @return TRUE if the function was registered successfully, or
 *         FALSE otherwise.
 */
gboolean oul_plugin_ipc_register(OulPlugin *plugin, const char *command,
								  OulCallback func,
								  OulSignalMarshalFunc marshal,
								  OulValue *ret_value, int num_params, ...);

/**
 * Unregisters an IPC command in a plugin.
 *
 * @param plugin  The plugin to unregister the command from.
 * @param command The name of the command.
 */
void oul_plugin_ipc_unregister(OulPlugin *plugin, const char *command);

/**
 * Unregisters all IPC commands in a plugin.
 *
 * @param plugin The plugin to unregister the commands from.
 */
void oul_plugin_ipc_unregister_all(OulPlugin *plugin);

/**
 * Returns a list of value types used for an IPC command.
 *
 * @param plugin     The plugin.
 * @param command    The name of the command.
 * @param ret_value  The returned return value.
 * @param num_params The returned number of parameters.
 * @param params     The returned list of parameters.
 *
 * @return TRUE if the command was found, or FALSE otherwise.
 */
gboolean oul_plugin_ipc_get_params(OulPlugin *plugin, const char *command,
									OulValue **ret_value, int *num_params,
									OulValue ***params);

/**
 * Executes an IPC command.
 *
 * @param plugin  The plugin to execute the command on.
 * @param command The name of the command.
 * @param ok      TRUE if the call was successful, or FALSE otherwise.
 * @param ...     The parameters to pass.
 *
 * @return The return value, which will be NULL if the command doesn't
 *         return a value.
 */
void *oul_plugin_ipc_call(OulPlugin *plugin, const char *command,
						   gboolean *ok, ...);

/*@}*/

/**************************************************************************/
/** @name Plugins API                                                     */
/**************************************************************************/
/*@{*/

/**
 * Add a new directory to search for plugins
 *
 * @param path The new search path.
 */
void oul_plugins_add_search_path(const char *path);

/**
 * Unloads all loaded plugins.
 */
void oul_plugins_unload_all(void);

/**
 * Destroys all registered plugins.
 */
void oul_plugins_destroy_all(void);

/**
 * Saves the list of loaded plugins to the specified preference key
 *
 * @param key The preference key to save the list of plugins to.
 */
void oul_plugins_save_loaded(const char *key);

/**
 * Attempts to load all the plugins in the specified preference key
 * that were loaded when oul last quit.
 *
 * @param key The preference key containing the list of plugins.
 */
void oul_plugins_load_saved(const char *key);

/**
 * Probes for plugins in the registered module paths.
 *
 * @param ext The extension type to probe for, or @c NULL for all.
 *
 * @see oul_plugin_set_probe_path()
 */
void oul_plugins_probe(const char *ext);

/**
 * Returns whether or not plugin support is enabled.
 *
 * @return TRUE if plugin support is enabled, or FALSE otherwise.
 */
gboolean oul_plugins_enabled(void);

#ifndef OUL_DISABLE_DEPRECATED
/**
 * Registers a function that will be called when probing is finished.
 *
 * @param func The callback function.
 * @param data Data to pass to the callback.
 * @deprecated If you need this, ask for a plugin-probe signal to be added.
 */
void oul_plugins_register_probe_notify_cb(void (*func)(void *), void *data);
#endif

#ifndef OUL_DISABLE_DEPRECATED
/**
 * Unregisters a function that would be called when probing is finished.
 *
 * @param func The callback function.
 * @deprecated If you need this, ask for a plugin-probe signal to be added.
 */
void oul_plugins_unregister_probe_notify_cb(void (*func)(void *));
#endif

#ifndef OUL_DISABLE_DEPRECATED
/**
 * Registers a function that will be called when a plugin is loaded.
 *
 * @param func The callback function.
 * @param data Data to pass to the callback.
 * @deprecated Use the plugin-load signal instead.
 */
void oul_plugins_register_load_notify_cb(void (*func)(OulPlugin *, void *),
										  void *data);
#endif

#ifndef OUL_DISABLE_DEPRECATED
/**
 * Unregisters a function that would be called when a plugin is loaded.
 *
 * @param func The callback function.
 * @deprecated Use the plugin-load signal instead.
 */
void oul_plugins_unregister_load_notify_cb(void (*func)(OulPlugin *, void *));
#endif

#ifndef OUL_DISABLE_DEPRECATED
/**
 * Registers a function that will be called when a plugin is unloaded.
 *
 * @param func The callback function.
 * @param data Data to pass to the callback.
 * @deprecated Use the plugin-unload signal instead.
 */
void oul_plugins_register_unload_notify_cb(void (*func)(OulPlugin *, void *),
											void *data);
#endif

#ifndef OUL_DISABLE_DEPRECATED
/**
 * Unregisters a function that would be called when a plugin is unloaded.
 *
 * @param func The callback function.
 * @deprecated Use the plugin-unload signal instead.
 */
void oul_plugins_unregister_unload_notify_cb(void (*func)(OulPlugin *,
														   void *));
#endif

/**
 * Finds a plugin with the specified name.
 *
 * @param name The plugin name.
 *
 * @return The plugin if found, or @c NULL if not found.
 */
OulPlugin *oul_plugins_find_with_name(const char *name);

/**
 * Finds a plugin with the specified filename (filename with a path).
 *
 * @param filename The plugin filename.
 *
 * @return The plugin if found, or @c NULL if not found.
 */
OulPlugin *oul_plugins_find_with_filename(const char *filename);

/**
 * Finds a plugin with the specified basename (filename without a path).
 *
 * @param basename The plugin basename.
 *
 * @return The plugin if found, or @c NULL if not found.
 */
OulPlugin *oul_plugins_find_with_basename(const char *basename);

/**
 * Finds a plugin with the specified plugin ID.
 *
 * @param id The plugin ID.
 *
 * @return The plugin if found, or @c NULL if not found.
 */
OulPlugin *oul_plugins_find_with_id(const char *id);

/**
 * Returns a list of all loaded plugins.
 *
 * @constreturn A list of all loaded plugins.
 */
GList *oul_plugins_get_loaded(void);

/**
 * Returns a list of all valid protocol plugins.  A protocol
 * plugin is considered invalid if it does not contain the call
 * to the OUL_INIT_PLUGIN() macro, or if it was compiled
 * against an incompatable API version.
 *
 * @constreturn A list of all protocol plugins.
 */
GList *oul_plugins_get_protocols(void);

/**
 * Returns a list of all plugins, whether loaded or not.
 *
 * @constreturn A list of all plugins.
 */
GList *oul_plugins_get_all(void);

/*@}*/

/**************************************************************************/
/** @name Plugins SubSytem API                                            */
/**************************************************************************/
/*@{*/

/**
 * Returns the plugin subsystem handle.
 *
 * @return The plugin sybsystem handle.
 */
void *oul_plugins_get_handle(void);

/**
 * Initializes the plugin subsystem
 */
void oul_plugins_init(void);

/**
 * Uninitializes the plugin subsystem
 */
void oul_plugins_uninit(void);

/*@}*/

/**
 * Allocates and returns a new OulPluginAction.
 *
 * @param label    The description of the action to show to the user.
 * @param callback The callback to call when the user selects this action.
 */
OulPluginAction *oul_plugin_action_new(const char* label, void (*callback)(OulPluginAction *));

/**
 * Frees a OulPluginAction
 *
 * @param action The OulPluginAction to free.
 */
void oul_plugin_action_free(OulPluginAction *action);

#ifdef __cplusplus
}
#endif

#endif /* _OUL_PLUGIN_H_ */
