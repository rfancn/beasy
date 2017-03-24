#ifndef __BEASY_GTK_PREF_NET_H
#define __BEASY_GTK_PREF_NET_H

#include "gtkprefs.h"

#define	BEASY_PREFS_NET_BROWSERS_ROOT		BEASY_PREFS_ROOT "/browsers"
#define	BEASY_PREFS_NET_BROWSERS_PLACE		BEASY_PREFS_NET_BROWSERS_ROOT "/place"
#define	BEASY_PREFS_NET_BROWSERS_COMMAND 	BEASY_PREFS_NET_BROWSERS_ROOT "/command"
#define	BEASY_PREFS_NET_BROWSERS_BROWSER	BEASY_PREFS_NET_BROWSERS_ROOT "/browser"

GtkWidget *	beasy_net_prefpage_get(void);

#endif
