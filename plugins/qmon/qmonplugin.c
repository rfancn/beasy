#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib.h>

#include "qmonxml.h"

/* This is the required definition of OUL_PLUGINS as required for a plugin,
 * but we protect it with an #ifndef because config.h may define it for us
 * already and this would cause an unneeded compiler warning. */
#ifndef OUL_PLUGINS
# define OUL_PLUGINS
#endif

#define PLUGIN_ID 		"core_qmonitor"
#define PLUGIN_AUTHOR 	"Ryan Fan<rfan.cn@gmail.com>"

#include <proxy.h>

#include <notify.h>
#include <plugin.h>
#include <version.h>

#include "beasy.h"
#include "qmon.h"
#include "qmonplugin.h"
#include "qmonpref.h"
#include "gtkplugin.h"

static OulPlugin 	*plugin_qmon = NULL;
static QmonMonitor 	*monitor = NULL;

static void
plugin_qmon_start(OulPluginAction *action)
{
	oul_debug_info("qmonplugin", "qmon start.\n");

	if(monitor == NULL)
		monitor = qmon_monitor_new(plugin_qmon);

	qmon_monitor_start(monitor);
}

static void
plugin_qmon_stop(OulPluginAction *action)
{
	oul_debug_info("qmonplugin", "qmon stop.\n");
	
	qmon_monitor_stop(monitor);
}

static void
plugin_qmon_check(OulPluginAction *action)
{
	oul_debug_info("qmonplugin", "qmon check.\n");

	if(monitor == NULL)
		monitor = qmon_monitor_new(plugin_qmon);

	qmon_monitor_poll(monitor);
}

static GList *
plugin_actions(OulPlugin *plugin, gpointer context)
{
	GList *actions = NULL;

	/* Here we take advantage of return values to avoid the need for a temp variable */
	actions = g_list_prepend(actions,
		oul_plugin_action_new("QMon Monitor Start", plugin_qmon_start));

	actions = g_list_prepend(actions,
		oul_plugin_action_new("QMon Monitor Stop", plugin_qmon_stop));

	actions = g_list_prepend(actions,
		oul_plugin_action_new("QMon Monitor Check", plugin_qmon_check));

	return g_list_reverse(actions);
}

static gboolean
plugin_load(OulPlugin *plugin)
{
	gint interval;
	
	/* we need a handle for all the notify calls */
	plugin_qmon = plugin;

	return TRUE;
}

static gboolean
plugin_unload(OulPlugin *plugin)
{
	if(monitor)
		qmon_monitor_destroy(monitor);

	return TRUE;
}

static BeasyPluginUiInfo ui_info = {
	qmon_prefs_frame_get,	/* get function to retrieve the frame handler */
	0,						/* display which num of the pref page */
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
	BEASY_UI,                   /* UI requirement */
	0,                          /* flags */
	NULL,                       /* dependencies */
	OUL_PRIORITY_DEFAULT,    	/* priority */

	PLUGIN_ID,                  					/* id */
	"Qmon Monitor",       							/* name */
	"0.0.1",                    						/* version */
	"Check global updated SR queue in Oracle Qmon",		/* summary */
	"It will notify you when your have some update"
	"in your queue",					 				/* description */
	PLUGIN_AUTHOR,              						/* author */
	"http://rfan.512j.com",     						/* homepage */

	plugin_load,                /* load */
	plugin_unload,              /* unload */
	NULL,                       /* destroy */

	&ui_info,               	/* ui info */
	NULL,                		/* extra info */
	NULL,		                /* prefs info */
	plugin_actions,             /* actions */
	NULL,                       /* reserved */
	NULL,                       /* reserved */
	NULL,                       /* reserved */
	NULL                        /* reserved */
};

static void
init_plugin(OulPlugin *plugin)
{
	qmon_prefs_init();
}

OUL_INIT_PLUGIN("qmonplugin", init_plugin, info);

