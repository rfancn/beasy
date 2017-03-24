#ifndef _OUL_PLUGINPREF_H_
#define _OUL_PLUGINPREF_H_

typedef struct _OulPluginPrefFrame		OulPluginPrefFrame;
typedef struct _OulPluginPref			OulPluginPref;

/**
 * String format for preferences.
 */
typedef enum
{
	OUL_STRING_FORMAT_TYPE_NONE      = 0,          /**< The string is plain text. */
	OUL_STRING_FORMAT_TYPE_MULTILINE = 1 << 0,     /**< The string can have newlines. */
	OUL_STRING_FORMAT_TYPE_HTML      = 1 << 1      /**< The string can be in HTML. */
} OulStringFormatType;

typedef enum {
	OUL_PLUGIN_PREF_NONE,
	OUL_PLUGIN_PREF_CHOICE,
	OUL_PLUGIN_PREF_INFO,              /**< no-value label */
	OUL_PLUGIN_PREF_STRING_FORMAT      /**< The preference has a string value. */
} OulPluginPrefType;

#include <glib.h>
#include "prefs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/** @name Plugin Preference API                                           */
/**************************************************************************/
/*@{*/

/**
 * Create a new plugin preference frame
 *
 * @return a new OulPluginPrefFrame
 */
OulPluginPrefFrame *oul_plugin_pref_frame_new(void);

/**
 * Destroy a plugin preference frame
 *
 * @param frame The plugin frame to destroy
 */
void oul_plugin_pref_frame_destroy(OulPluginPrefFrame *frame);

/**
 * Adds a plugin preference to a plugin preference frame
 *
 * @param frame The plugin frame to add the preference to
 * @param pref  The preference to add to the frame
 */
void oul_plugin_pref_frame_add(OulPluginPrefFrame *frame, OulPluginPref *pref);

/**
 * Get the plugin preferences from a plugin preference frame
 *
 * @param frame The plugin frame to get the plugin preferences from
 * @constreturn a GList of plugin preferences
 */
GList *oul_plugin_pref_frame_get_prefs(OulPluginPrefFrame *frame);

/**
 * Create a new plugin preference
 *
 * @return a new OulPluginPref
 */
OulPluginPref *oul_plugin_pref_new(void);

/**
 * Create a new plugin preference with name
 *
 * @param name The name of the pref
 * @return a new OulPluginPref
 */
OulPluginPref *oul_plugin_pref_new_with_name(const char *name);

/**
 * Create a new plugin preference with label
 *
 * @param label The label to be displayed
 * @return a new OulPluginPref
 */
OulPluginPref *oul_plugin_pref_new_with_label(const char *label);

/**
 * Create a new plugin preference with name and label
 *
 * @param name  The name of the pref
 * @param label The label to be displayed
 * @return a new OulPluginPref
 */
OulPluginPref *oul_plugin_pref_new_with_name_and_label(const char *name, const char *label);

/**
 * Destroy a plugin preference
 *
 * @param pref The preference to destroy
 */
void oul_plugin_pref_destroy(OulPluginPref *pref);

/**
 * Set a plugin pref name
 *
 * @param pref The plugin pref
 * @param name The name of the pref
 */
void oul_plugin_pref_set_name(OulPluginPref *pref, const char *name);

/**
 * Get a plugin pref name
 *
 * @param pref The plugin pref
 * @return The name of the pref
 */
const char *oul_plugin_pref_get_name(OulPluginPref *pref);

/**
 * Set a plugin pref label
 *
 * @param pref  The plugin pref
 * @param label The label for the plugin pref
 */
void oul_plugin_pref_set_label(OulPluginPref *pref, const char *label);

/**
 * Get a plugin pref label
 *
 * @param pref The plugin pref
 * @return The label for the plugin pref
 */
const char *oul_plugin_pref_get_label(OulPluginPref *pref);

/**
 * Set the bounds for an integer pref
 *
 * @param pref The plugin pref
 * @param min  The min value
 * @param max  The max value
 */
void oul_plugin_pref_set_bounds(OulPluginPref *pref, int min, int max);

/**
 * Get the bounds for an integer pref
 *
 * @param pref The plugin pref
 * @param min  The min value
 * @param max  The max value
 */
void oul_plugin_pref_get_bounds(OulPluginPref *pref, int *min, int *max);

/**
 * Set the type of a plugin pref
 *
 * @param pref The plugin pref
 * @param type The type
 */
void oul_plugin_pref_set_type(OulPluginPref *pref, OulPluginPrefType type);

/**
 * Get the type of a plugin pref
 *
 * @param pref The plugin pref
 * @return The type
 */
OulPluginPrefType oul_plugin_pref_get_type(OulPluginPref *pref);

/**
 * Set the choices for a choices plugin pref
 *
 * @param pref  The plugin pref
 * @param label The label for the choice
 * @param choice  A gpointer of the choice
 */
void oul_plugin_pref_add_choice(OulPluginPref *pref, const char *label, gpointer choice);

/**
 * Get the choices for a choices plugin pref
 *
 * @param pref The plugin pref
 * @constreturn GList of the choices
 */
GList *oul_plugin_pref_get_choices(OulPluginPref *pref);

/**
 * Set the max length for a string plugin pref
 *
 * @param pref       The plugin pref
 * @param max_length The max length of the string
 */
void oul_plugin_pref_set_max_length(OulPluginPref *pref, unsigned int max_length);

/**
 * Get the max length for a string plugin pref
 *
 * @param pref The plugin pref
 * @return the max length
 */
unsigned int oul_plugin_pref_get_max_length(OulPluginPref *pref);

/**
 * Sets the masking of a string plugin pref
 *
 * @param pref The plugin pref
 * @param mask The value to set
 */
void oul_plugin_pref_set_masked(OulPluginPref *pref, gboolean mask);

/**
 * Gets the masking of a string plugin pref
 *
 * @param pref The plugin pref
 * @return The masking
 */
gboolean oul_plugin_pref_get_masked(OulPluginPref *pref);

/**
 * Sets the format type for a formattable-string plugin pref. You need to set the
 * pref type to OUL_PLUGIN_PREF_STRING_FORMAT first before setting the format.
 *
 * @param pref	 The plugin pref
 * @param format The format of the string
 */
void oul_plugin_pref_set_format_type(OulPluginPref *pref, OulStringFormatType format);

/**
 * Gets the format type of the formattable-string plugin pref.
 *
 * @param pref The plugin pref
 * @return The format of the pref
 */
OulStringFormatType oul_plugin_pref_get_format_type(OulPluginPref *pref);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _OUL_PLUGINPREF_H_ */
