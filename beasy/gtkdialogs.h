/**
 * @defgroup beasy Beasy (GTK+ User Interface)
 */

#ifndef __BEASY_DIALOGS_H_
#define __BEASY_DIALOGS_H_

#include "beasy.h"

/* Functions in gtkdialogs.c (these should actually stay in this file) */
void beasy_dialogs_destroy_all(void);
void beasy_dialogs_about(void);
void beasy_dialogs_im(void);
void beasy_dialogs_info(void);
void beasy_dialogs_log(void);

#define BEASY_WINDOW_ICONIFIED(x) (gdk_window_get_state(GTK_WIDGET(x)->window) & GDK_WINDOW_STATE_ICONIFIED)

#endif /* __BEASY_DIALOGS_H_ */
