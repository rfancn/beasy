#ifndef _BEASY_H_
#define _BEASY_H_

#include <glib.h>
#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11
# include <gdk/gdkx.h>
#endif

#define BEASY_UI 			"beasy_ui" 
#define BEASY_NAME 			"haha beasy name" 
#define BEASY_ALERT_TITLE 	""

/*
 * Spacings between components, as defined by the
 * GNOME Human Interface Guidelines.
 */
#define BEASY_HIG_CAT_SPACE     18
#define BEASY_HIG_BORDER        12
#define BEASY_HIG_BOX_SPACE      6

/*
 * See GNOME bug #307304 for some discussion about the invisible
 * character.  0x25cf is a good choice, too.
 */
#define BEASY_INVISIBLE_CHAR (gunichar)0x2022


#endif
