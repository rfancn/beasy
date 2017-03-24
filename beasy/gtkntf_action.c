#include <gtk/gtk.h>

#include "internal.h"

#include "debug.h"
#include "plugin.h"
#include "version.h"

#include "gtkntf_action.h"
#include "gtkntf_display.h"
#include "gtkntf_event.h"
#include "gtkntf_event_info.h"
#include "gtkntf_notification.h"
#include "gtkntf_utils.h"

#include "msg.h"

struct _GtkNtfAction {
	gchar *name;
	gchar *i18n;
	GtkNtfActionFunc func;
};

static GList *actions = NULL;

/*******************************************************************************
 * API
 ******************************************************************************/
GtkNtfAction *
gtkntf_action_new() {
	GtkNtfAction *action;
	
	action = g_new0(GtkNtfAction, 1);

	return action;
}

void
gtkntf_action_destroy(GtkNtfAction *action) {
	g_return_if_fail(action);

	if(action->name)
		g_free(action->name);

	g_free(action);
	action = NULL;
}

void
gtkntf_action_set_name(GtkNtfAction *action, const gchar *name) {
	g_return_if_fail(action);
	g_return_if_fail(name);

	if(action->name)
		g_free(action->name);

	action->name = g_strdup(name);
}

const gchar *
gtkntf_action_get_name(GtkNtfAction *action) {
	g_return_val_if_fail(action, NULL);

	return action->name;
}

void
gtkntf_action_set_i18n(GtkNtfAction *action, const gchar *i18n) {
	g_return_if_fail(action);
	g_return_if_fail(i18n);

	if(action->i18n)
		g_free(action->i18n);

	action->i18n = g_strdup(i18n);
}

const gchar *
gtkntf_action_get_i18n(GtkNtfAction *action) {
	g_return_val_if_fail(action, NULL);

	return action->i18n;
}

void
gtkntf_action_set_func(GtkNtfAction *action, GtkNtfActionFunc func) {
	g_return_if_fail(action);
	g_return_if_fail(func);

	action->func = func;
}

GtkNtfActionFunc
gtkntf_action_get_func(GtkNtfAction *action) {
	g_return_val_if_fail(action, NULL);

	return action->func;
}

void
gtkntf_action_execute(GtkNtfAction *action, GtkNtfDisplay *display, GdkEventButton *event) {
	g_return_if_fail(action);
	g_return_if_fail(display);

	action->func(display, event);
}

GtkNtfAction *
gtkntf_action_find_with_name(const gchar *name) {
	GtkNtfAction *action;
	GList *l;

	g_return_val_if_fail(name, NULL);

	for(l = actions; l; l = l->next) {
		action = GTKNTF_ACTION(l->data);

		if(!g_ascii_strcasecmp(name, action->name))
			return action;
	}

	return NULL;
}

GtkNtfAction *
gtkntf_action_find_with_i18n(const gchar *i18n) {
	GtkNtfAction *action;
	GList *l;

	g_return_val_if_fail(i18n, NULL);

	for(l = actions; l; l = l->next) {
		action = GTKNTF_ACTION(l->data);

		if(!g_ascii_strcasecmp(i18n, action->i18n))
			return action;
	}

	return NULL;
}

gint
gtkntf_action_get_position(GtkNtfAction *action) {
	g_return_val_if_fail(action, -1);

	return g_list_index(actions, action);
}

/*******************************************************************************
 * Sub System
 ******************************************************************************/
static void
gtkntf_action_add_default(const gchar *name, const gchar *i18n, GtkNtfActionFunc func) {
	GtkNtfAction *action;

	g_return_if_fail(name);
	g_return_if_fail(func);

	action = gtkntf_action_new();
	gtkntf_action_set_name(action, name);
	gtkntf_action_set_i18n(action, i18n);
	gtkntf_action_set_func(action, func);

	gtkntf_actions_add_action(action);
}

void
gtkntf_actions_init() {

	gtkntf_action_add_default("close", _("Close"), gtkntf_action_execute_close);

	gtkntf_action_add_default("info", _("Display Detail Info"), gtkntf_action_execute_info);
}

void
gtkntf_actions_uninit() {
	GList *l, *ll;

	for(l = actions; l; l = ll) {
		ll = l->next;

		gtkntf_actions_remove_action(GTKNTF_ACTION(l->data));
	}

	g_list_free(actions);
	actions = NULL;
}

void
gtkntf_actions_add_action(GtkNtfAction *action) {
	g_return_if_fail(action);

	actions = g_list_append(actions, action);
}

void
gtkntf_actions_remove_action(GtkNtfAction *action) {
	g_return_if_fail(action);

	actions = g_list_remove(actions, action);
}

gint
gtkntf_actions_count() {
	return g_list_length(actions);
}

const gchar *
gtkntf_actions_get_nth_name(gint nth) {
	GtkNtfAction *action;

	action = GTKNTF_ACTION(g_list_nth_data(actions, nth));

	return action->name;
}

const gchar *
gtkntf_actions_get_nth_i18n(gint nth) {
	GtkNtfAction *action;

	action = GTKNTF_ACTION(g_list_nth_data(actions, nth));

	return action->i18n;
}

/*******************************************************************************
 * Action Functions
 ******************************************************************************/
void
gtkntf_action_execute_close(GtkNtfDisplay *display, GdkEventButton *gdk_event) {
	g_return_if_fail(display);

	gtkntf_display_destroy(display);
}

static void
gtkntf_notify_close_cb(gpointer data)
{
	OulMsg *msg = (OulMsg *)data;

	oul_msg_destroy(msg);
}

void
gtkntf_action_execute_info(GtkNtfDisplay *display, GdkEventButton *gdk_event) {
	OulPlugin *plugin;
	GtkNtfEventInfo *info;
	int handle;

	g_return_if_fail(display);

	info = gtkntf_display_get_event_info(display);

	const gchar *source = gtkntf_event_info_get_source(info);
	const gchar *title = gtkntf_event_info_get_title(info);
	
	if(source)
		plugin = oul_plugins_find_with_name(source);

	if(plugin && plugin->extra){
		OulMsg *msg = xmlmsg_to_oulmsg(plugin->extra, strlen(plugin->extra));
		if(msg == NULL)
			return;
	
		switch(msg->ctype){
			case OULMSG_CTYPE_TEXT:
			case OULMSG_CTYPE_MARKUP:
				oul_debug_info("gtkntf_action", "TEXT\n");
				oul_notify_formatted(&handle, NULL, title, NULL, 
									msg->content.text, 320, 240, NULL, NULL);
				break;
			case OULMSG_CTYPE_TABLE:
				oul_debug_info("gtkntf_action", "TABLE\n");
				oul_notify_table(&handle, title, msg, NULL, NULL);
				break;
		}

		g_free(plugin->extra);
		plugin->extra = NULL;

		if(msg)
			oul_msg_destroy(msg);
		
	}
	
}

/******************************************************************************
 * Context menu stuff
 *****************************************************************************/
static gboolean
gtkntf_action_context_destroy_cb(gpointer data) {
	GtkNtfDisplay *display = GTKNTF_DISPLAY(data);

	gtkntf_display_destroy(display);

	return FALSE;
}

#if 0
static void
gtkntf_action_context_hide_cb(GtkWidget *w, gpointer data) {
	GtkNtfDisplay *display = GTKNTF_DISPLAY(data);
	GtkNtfEventInfo *info = NULL;
	gint display_time;
	guint timeout_id;

	g_return_if_fail(display);

	info = gtkntf_display_get_event_info(display);
	g_return_if_fail(info);

	display_time = oul_prefs_get_int(BEASY_PREFS_NTF_BEHAVIOR_DISPLAY_TIME);
	timeout_id = g_timeout_add(display_time * 500,
							   gtkntf_action_context_destroy_cb, display);

	gtkntf_event_info_set_timeout_id(info, timeout_id);
}
#endif

static void
gtkntf_action_context_position(GtkMenu *menu, gint *x, gint *y, gboolean pushin,
						   gpointer data)
{
	GtkRequisition req;
	gint scrheight = 0;

	scrheight = gdk_screen_get_height(gtk_widget_get_screen(GTK_WIDGET(menu)));

	gtk_widget_size_request(GTK_WIDGET(menu), &req);

	if((*y + req.height > scrheight) && (scrheight - req.height > 0))
			*y = scrheight - req.height;
}

static void
gtkntf_action_context_info_cb(GtkWidget *menuitem, GtkNtfDisplay *display) {
	gtkntf_action_execute_info(display, NULL);
}

/* This function has come from hell.. I summoned it last time I sent a wicked
 * soul to it's eternal resting place.
 */
void
gtkntf_action_execute_context(GtkNtfDisplay *display, GdkEventButton *gdk_event) {
	GtkNtfEventInfo *info = NULL;
	GtkWidget *menu;
	guint timeout_id;

	g_return_if_fail(display);

	/* grab the stuff we need from the display and event info */
	info = gtkntf_display_get_event_info(display);
	g_return_if_fail(info);

	/* we're going to show the menu as long as the timeout is removed.
	 * We need to remove it otherwise the display get's destroyed when
	 * the menu is shown, or it'll leak if we don't clean it up later.
	 */
	timeout_id = gtkntf_event_info_get_timeout_id(info);
	g_return_if_fail(g_source_remove(timeout_id));

	/* create the menu */
	menu = gtk_menu_new();

#if 0
	g_signal_connect(G_OBJECT(menu), "hide",
					 G_CALLBACK(gtkntf_action_context_hide_cb), display);
	gk_widget_show(menu);
#endif

	/* show it already */
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
				   (GtkMenuPositionFunc)gtkntf_action_context_position, display,
				   gdk_event->button, gdk_event->time);
}

