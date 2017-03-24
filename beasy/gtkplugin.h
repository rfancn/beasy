/**
 * @file gtkplugin.h GTK+ Plugin API
 * @ingroup beasy
 */

#ifndef _PIDGINPLUGIN_H_
#define _PIDGINPLUGIN_H_

#include "beasy.h"
#include "plugin.h"

typedef struct _BeasyPluginUiInfo BeasyPluginUiInfo;

/**
 * A GTK+ UI structure for plugins.
 */
struct _BeasyPluginUiInfo
{
	GtkWidget *(*get_config_frame)(OulPlugin *plugin);

	int page_num;                                         /**< Reserved */

	/* padding */
	void (*_beasy_reserved1)(void);
	void (*_beasy_reserved2)(void);
	void (*_beasy_reserved3)(void);
	void (*_beasy_reserved4)(void);
};

#define BEASY_PLUGIN_TYPE BEASY_UI

#define BEASY_IS_PIDGIN_PLUGIN(plugin) \
	((plugin)->info != NULL && (plugin)->info->ui_info != NULL && \
	 !strcmp((plugin)->info->ui_requirement, BEASY_PLUGIN_TYPE))

#define BEASY_PLUGIN_UI_INFO(plugin) \
	((BeasyPluginUiInfo *)(plugin)->info->ui_info)

/**
 * Returns the configuration frame widget for a GTK+ plugin, if one
 * exists.
 *
 * @param plugin The plugin.
 *
 * @return The frame, if the plugin is a GTK+ plugin and provides a
 *         configuration frame.
 */
GtkWidget *beasy_plugin_get_config_frame(OulPlugin *plugin);

/**
 * Saves all loaded plugins.
 */
void beasy_plugins_save(void);


/**
 * Shows the Plugins dialog
 */
void beasy_plugin_dialog_show(void);

#endif /* _PIDGINPLUGIN_H_ */
