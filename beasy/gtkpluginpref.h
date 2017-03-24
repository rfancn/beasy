/**
 * @file gtkpluginpref.h GTK+ Plugin Preferences
 * @ingroup beasy
 */

#ifndef _BEASYPLUGINPREF_H_
#define _BEASYPLUGINPREF_H_

#include "pluginpref.h"

#include "beasy.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a Gtk Preference frame for a OulPluginPrefFrame
 *
 * @param frame OulPluginPrefFrame
 * @return The gtk preference frame
 */
GtkWidget *beasy_plugin_pref_create_frame(OulPluginPrefFrame *frame);

#ifdef __cplusplus
}
#endif

#endif /* _BEASYPLUGINPREF_H_ */
