#ifndef _GTKDOCKLET_H_
#define _GTKDOCKLET_H_

#include "status.h"

struct docklet_ui_ops
{
	void (*create)(void);
	void (*destroy)(void);
	void (*update_icon)(OulStatusPrimitive, gboolean, gboolean);
	void (*blank_icon)(void);
	void (*set_tooltip)(gchar *);
	GtkMenuPositionFunc position_menu;
};


/* functions in gtkdocklet.c */
void    beasy_docklet_update_icon(void);
void    beasy_docklet_clicked(int);
void    beasy_docklet_embedded(void);
void    beasy_docklet_remove(void);
void    beasy_docklet_set_ui_ops(struct docklet_ui_ops *);
void    beasy_docklet_unload(void);
void    beasy_docklet_init(void);
void    beasy_docklet_uninit(void);
void*   beasy_docklet_get_handle(void);

/* function in gtkdocklet-{x11,win32}.c */
void docklet_ui_init(void);

#endif /* _GTKDOCKLET_H_ */
