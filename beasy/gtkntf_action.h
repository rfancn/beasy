#ifndef GTKNTF_ACTION_H
#define GTKNTF_ACTION_H

#define GTKNTF_ACTION(obj)	((GtkNtfAction *)(obj))

typedef struct _GtkNtfAction GtkNtfAction;

#include "gtkntf_display.h"

typedef void (*GtkNtfActionFunc)(GtkNtfDisplay *display, GdkEventButton *button);

G_BEGIN_DECLS

/* Api */
GtkNtfAction *		gtkntf_action_new();
void 				gtkntf_action_destroy(GtkNtfAction *action);
void 				gtkntf_action_set_name(GtkNtfAction *action, const gchar *name);
const gchar *		gtkntf_action_get_name(GtkNtfAction *action);
void 				gtkntf_action_set_i18n(GtkNtfAction *action, const gchar *i18n);
const gchar *		gtkntf_action_get_i18n(GtkNtfAction *action);
void		 		gtkntf_action_set_func(GtkNtfAction *action, GtkNtfActionFunc func);
GtkNtfActionFunc 	gtkntf_action_get_func(GtkNtfAction *action);
void 				gtkntf_action_execute(GtkNtfAction *action, GtkNtfDisplay *display, GdkEventButton *event);
GtkNtfAction *		gtkntf_action_find_with_name(const gchar *name);
GtkNtfAction *		gtkntf_action_find_with_i18n(const gchar *i18n);
gint 				gtkntf_action_get_position(GtkNtfAction *action);

/* Sub System */
void 				gtkntf_actions_init();
void 				gtkntf_actions_uninit();
void 				gtkntf_actions_add_action(GtkNtfAction *action);
void 				gtkntf_actions_remove_action(GtkNtfAction *action);
gint 				gtkntf_actions_count();
const gchar *		gtkntf_actions_get_nth_name(gint nth);
const gchar *		gtkntf_actions_get_nth_i18n(gint nth);

/* Action Functions */
void 				gtkntf_action_execute_close(GtkNtfDisplay *display, GdkEventButton *gdk_event);
void 				gtkntf_action_execute_open_conv(GtkNtfDisplay *display, GdkEventButton *gdk_event);
void 				gtkntf_action_execute_context(GtkNtfDisplay *display, GdkEventButton *gdk_event);
void 				gtkntf_action_execute_info(GtkNtfDisplay *display, GdkEventButton *gdk_event);
void 				gtkntf_action_execute_log(GtkNtfDisplay *display, GdkEventButton *gdk_event);

G_END_DECLS

#endif /* GTKNTF_ACTION_H */
