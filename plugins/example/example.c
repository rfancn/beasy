#include "internal.h"

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib.h>
#include "gtkntf.h"
#include "sound.h"

/* This will prevent compiler errors in some instances and is better explained in the
 * how-to documents on the wiki */
#ifndef G_GNUC_NULL_TERMINATED
# if __GNUC__ >= 4
#  define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
# else
#  define G_GNUC_NULL_TERMINATED
# endif
#endif

/* This is the required definition of OUL_PLUGINS as required for a plugin,
 * but we protect it with an #ifndef because config.h may define it for us
 * already and this would cause an unneeded compiler warning. */
#ifndef OUL_PLUGINS
# define OUL_PLUGINS
#endif

#define PLUGIN_ID "core-pluginexample"
#define PLUGIN_AUTHOR "Ryan Fan<rfan.cn@gmail.com>"

#include <proxy.h>
#include <dnsquery.h>

#include <notify.h>
#include <plugin.h>
#include <version.h>

static OulPlugin *plugin_example = NULL;

/* The next four functions and the calls within them should cause dialog boxes to appear
 * when you select the plugin action from the Tools->Notify Example menu */
static void
notify_error_cb(OulPluginAction *action)
{
	oul_signal_emit(oul_notify_get_handle(), "notify-error", "Example", "Error Title", "Error Message");
	
	oul_notify_error(plugin_example, "Test Notification", "Test Notification",
		"This is a test error notification");
}

static void
notify_info_cb(OulPluginAction *action)
{
	oul_signal_emit(oul_notify_get_handle(), "notify-info", "Example", "Title", "Body");
	oul_notify_info(plugin_example, "Test Notification", "Test Notification",
		"This is a test informative notification");
}

static void
notify_warn_cb(OulPluginAction *action)
{
	oul_signal_emit(oul_notify_get_handle(), "notify-warn", "Example", "Warn Title", "Warn Message");
	oul_notify_warning(plugin_example, "Test Notification", "Test Notification",
		"This is a test warning notification");
}

static void
notify_format_cb(OulPluginAction *action)
{
	oul_notify_formatted(plugin_example, "Test Notification", "Test Notification",
		"Test Notification",
		"<span color=\"red\" size=\"large\">Red text</span> is <i>cool</i>!<hr>"
		"<span color=\"blue\" size=\"large\">Blue text</span> is <i>cool</i>!<hr>"
		"http://www.163.com\n", 0, 0,
		NULL, NULL);
}

static void
notify_uri_cb(OulPluginAction *action)
{
	/* This one should open your web browser of choice. */
	oul_notify_uri(plugin_example, "http://rfan.512j.com/");
}

static void
test_sound_cb(OulPluginAction *action)
{
	const char *soundfile = "/usr/share/sounds/phone.wav";
	
	if(g_file_test(soundfile, G_FILE_TEST_EXISTS))
		oul_sound_play_file(soundfile);
	else
		oul_notify_error(plugin_example, 
				"Test Sound", "Test Notification",
				"/usr/share/sounds/phone.wav can not be found!");
}


static GList *
plugin_actions(OulPlugin *plugin, gpointer context)
{
	GList *actions = NULL;

	actions = g_list_prepend(actions,
		oul_plugin_action_new("Show Info Notification", notify_info_cb));

	/* Here we take advantage of return values to avoid the need for a temp variable */
	actions = g_list_prepend(actions,
		oul_plugin_action_new("Show Error Notification", notify_error_cb));

	actions = g_list_prepend(actions,
		oul_plugin_action_new("Show Warning Notification", notify_warn_cb));

	actions = g_list_prepend(actions,
		oul_plugin_action_new("Show Formatted Notification", notify_format_cb));

	actions = g_list_prepend(actions,
		oul_plugin_action_new("Show URI Notification", notify_uri_cb));

	actions = g_list_prepend(actions,
		oul_plugin_action_new("Sound Playing Test", test_sound_cb));


	return g_list_reverse(actions);
}

static gboolean
plugin_load(OulPlugin *plugin)
{
	plugin_example = plugin;


	return TRUE;
}

static gboolean
plugin_unload(OulPlugin *plugin)
{
	return TRUE;
}

typedef enum
{
	STATUS_NEVER,
	STATUS_ALWAYS,
	STATUS_FALLBACK
} UseStatusMessage;

static OulPluginPrefFrame * get_plugin_pref_frame()
{
	OulPluginPrefFrame *frame;
	OulPluginPref *pref;

	frame = oul_plugin_pref_frame_new();

	pref = oul_plugin_pref_new_with_name_and_label("/plugins/core/example/notify",
						_("Test Choice"));

	oul_plugin_pref_set_type(pref, OUL_PLUGIN_PREF_CHOICE);

	oul_plugin_pref_add_choice(pref, _("1"),
						GINT_TO_POINTER(STATUS_NEVER));
	oul_plugin_pref_add_choice(pref, _("2"),
						GINT_TO_POINTER(STATUS_ALWAYS));
	oul_plugin_pref_add_choice(pref, _("3"),
						GINT_TO_POINTER(STATUS_FALLBACK));

	oul_plugin_pref_frame_add(frame, pref);

	return frame;
}

static OulPluginUiInfo plugin_pref = {
	get_plugin_pref_frame,		/* get function to retrieve the frame handler */
	0,							/* display which num of the pref page */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static OulPluginInfo info = {
	OUL_PLUGIN_MAGIC,        	/* magic number */
	OUL_MAJOR_VERSION,       	/* oul major */
	OUL_MINOR_VERSION,       	/* oul minor */
	OUL_PLUGIN_STANDARD,     	/* plugin type */
	NULL,                       /* UI requirement */
	0,                          /* flags */
	NULL,                       /* dependencies */
	OUL_PRIORITY_DEFAULT,    	/* priority */

	PLUGIN_ID,                  						/* id */
	"Example",       									/* name */
	"0.0.1",                    						/* version */
	"ALL API Test",       								/* summary */
	"All API Test"
	"it will check all APIs oul provided", 				/* description */
	PLUGIN_AUTHOR,              						/* author */
	"http://rfan.512j.com",     						/* homepage */

	plugin_load,                /* load */
	plugin_unload,              /* unload */
	NULL,                       /* destroy */

	NULL,		                /* ui info */
	NULL,                       /* extra info */
	&plugin_pref,               /* prefs info */
	plugin_actions,             /* actions */
	NULL,                       /* reserved */
	NULL,                       /* reserved */
	NULL,                       /* reserved */
	NULL                        /* reserved */
};

static void
init_plugin(OulPlugin *plugin)
{
	oul_prefs_add_none("/plugins/core/example");
	oul_prefs_add_int("/plugins/core/example/notify", 1);
}

OUL_INIT_PLUGIN("plugin-examples", init_plugin, info)

