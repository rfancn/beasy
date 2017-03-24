#ifndef __PLUGIN_QMON_PREF_H
#define __PLUGIN_QMON_PREF_H

#include <gtk/gtk.h>
#include "plugin.h"

typedef enum
{
	QMON_TARGET_CTC = 0,
	QMON_TARGET_ANALYST,
	QMON_TARGET_SRLIST,
	QMON_TARGET_NUM
}QmonTarget;

#define PLUGIN_QMON_ROOT		"/plugins/core/qmonitor"
#define	PLUGIN_QMON_USERNAME	PLUGIN_QMON_ROOT "/username"
#define	PLUGIN_QMON_PASSWORD	PLUGIN_QMON_ROOT "/password"
#define	PLUGIN_QMON_SELECTION	PLUGIN_QMON_ROOT "/selection"

#define PLUGIN_QMON_INTERVAL	PLUGIN_QMON_ROOT "/interval"

#define PLUGIN_QMON_TARGET		PLUGIN_QMON_ROOT "/target"

/* Perfsonal Queue Monitor Parameters */
#define	PLUGIN_QMON_ANALYST		PLUGIN_QMON_ROOT "/analyst"

/* SR list monitor parameters */
#define	PLUGIN_QMON_SRLIST		PLUGIN_QMON_ROOT "/srlist"

GtkWidget *	qmon_prefs_frame_get(OulPlugin *plugin);
void		qmon_prefs_init();


#endif

