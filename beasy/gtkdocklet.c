#include "internal.h"
#include "beasy.h"

#include "core.h"
#include "debug.h"
#include "prefs.h"
#include "signals.h"

#include "status.h"
#include "savedstatuses.h"

#include "gtkprefs.h"
#include "gtkplugin.h"
#include "gtkdocklet.h"
#include "beasystock.h"

#include "gtkpref_sound.h"

#ifndef DOCKLET_TOOLTIP_LINE_LIMIT
#define DOCKLET_TOOLTIP_LINE_LIMIT 5
#endif

/* globals */
static int handle = 0; 
static struct docklet_ui_ops *ui_ops = NULL;
static OulStatusPrimitive status = OUL_STATUS_OFFLINE;
static gboolean pending = FALSE;
static gboolean connecting = FALSE;
static gboolean enable_join_chat = FALSE;
static guint docklet_blinking_timer = 0;
static gboolean visible = FALSE;

/**************************************************************************
 * private functions definitions
 **************************************************************************/
static gboolean 	docklet_blink_icon(gpointer data);
static GList *		get_pending_list(guint max);
static gboolean		docklet_update_status(void);
static void 		build_plugin_actions(GtkWidget *menu, OulPlugin *plugin, gpointer context);
static void 		plugin_act_cb(GtkObject *obj, OulPluginAction *pam);



/**************************************************************************
 * docklet status and utility functions
 **************************************************************************/

static gboolean
docklet_blink_icon(gpointer data)
{
	static gboolean blinked = FALSE;
	gboolean ret = FALSE; /* by default, don't keep blinking */

	blinked = !blinked;

	if(pending && !connecting) {
		if (blinked) {
			if (ui_ops && ui_ops->blank_icon)
				ui_ops->blank_icon();
		} else {
			beasy_docklet_update_icon();
		}
		ret = TRUE; /* keep blinking */
	} else {
		docklet_blinking_timer = 0;
		blinked = FALSE;
	}

	return ret;
}

static GList *
get_pending_list(guint max)
{
	GList *list_msgs = NULL;
        
    /* get unseen message list */
    /* list_msgs = beasy_messages_find _unseen_list(
            MessageType type, BeasyUnseenState min_state,
            gboolean hidden_only, gunint max_count) */ 

    return list_msgs;
}

static gboolean
docklet_update_status(void)
{
	GList *msgs, *l;
	int count;
    OulSavedStatus *saved_status;
	OulStatusPrimitive newstatus = OUL_STATUS_OFFLINE;
	gboolean newpending = FALSE, newconnecting = FALSE;

	/* get the current savedstatus */
	saved_status = oul_savedstatus_get_current();

	/* determine if any plugins have unread messages */
	msgs = get_pending_list(DOCKLET_TOOLTIP_LINE_LIMIT);

	if (msgs != NULL) {
		newpending = TRUE;

		/* set tooltip if messages are pending */
		if (ui_ops->set_tooltip) {
			GString *tooltip_text = g_string_new("");
			for (l = msgs, count = 0 ; l != NULL ; l = l->next, count++) {
                /**
				OulMessage *msg = (OulConversation *)l->data;
				BeasyMessage *gtkmsg = BEASY_CONVERSATION(conv);

				if (count == DOCKLET_TOOLTIP_LINE_LIMIT - 1) {
					g_string_append(tooltip_text, _("Right-click for more unread messages...\n"));
				} else if(gtkmsg) {
					g_string_append_printf(tooltip_text,
						ngettext("%d unread message from %s\n", "%d unread messages from %s\n", gtkmsg->unseen_count),
						gtkmsg->unseen_count,
						oul_conversation_get_title(conv));
				} else {
					g_string_append_printf(tooltip_text,
						ngettext("%d unread message from %s\n", "%d unread messages from %s\n",
						GPOINTER_TO_INT(oul_message_get_data(conv, "unseen-count"))),
						GPOINTER_TO_INT(oul_message_get_data(conv, "unseen-count")),
						oul_message_get_title(conv));
				}
                		**/
			}

			/* get rid of the last newline */
			if (tooltip_text->len > 0)
				tooltip_text = g_string_truncate(tooltip_text, tooltip_text->len - 1);

			ui_ops->set_tooltip(tooltip_text->str);

			g_string_free(tooltip_text, TRUE);
		}

		g_list_free(msgs);

	} else if (ui_ops->set_tooltip) {
		char *tooltip_text = g_strconcat(BEASY_NAME, " - ",
			oul_savedstatus_get_title(saved_status), NULL);
		ui_ops->set_tooltip(tooltip_text);
		g_free(tooltip_text);
	}

	newstatus = oul_savedstatus_get_type(saved_status);

	/* update the icon if we changed status */
	if (status != newstatus || pending!=newpending || connecting!=newconnecting) {
		status = newstatus;
		pending = newpending;
		connecting = newconnecting;

		beasy_docklet_update_icon();

		/* and schedule the blinker function if messages are pending */
		if (pending && !connecting && docklet_blinking_timer == 0) {
			docklet_blinking_timer = g_timeout_add(500, docklet_blink_icon, NULL);
		}
	}

	return FALSE; /* for when we're called by the glib idle handler */
}

static gboolean
online_account_supports_chat(void)
{
#ifdef ajdsfiojasdfj
	GList *c = NULL;
	c = oul_connections_get_all();

	while(c != NULL) {
		OulConnection *gc = c->data;
		OulPluginProtocolInfo *prpl_info = OUL_PLUGIN_PROTOCOL_INFO(gc->prpl);
		if (prpl_info != NULL && prpl_info->chat_info != NULL)
			return TRUE;
		c = c->next;
	}

	return FALSE;
#endif
}

/**************************************************************************
 * callbacks and signal handlers
 **************************************************************************/
#if 0
static void
beasy_quit_cb()
{
	/* TODO: confirm quit while pending */
}
#endif

static void
docklet_update_status_cb(void *data)
{
	docklet_update_status();
}

#ifdef asdfjoiasdjf
static void
docklet_conv_updated_cb(OulConversation *conv, OulConvUpdateType type)
{
	if (type == OUL_CONV_UPDATE_UNSEEN)
		docklet_update_status();
}

static void
docklet_signed_on_cb(OulConnection *gc)
{
	if (!enable_join_chat) {
		if (OUL_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info != NULL)
			enable_join_chat = TRUE;
	}
	docklet_update_status();
}

static void
docklet_signed_off_cb(OulConnection *gc)
{
	if (enable_join_chat) {
		if (OUL_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info != NULL)
			enable_join_chat = online_account_supports_chat();
	}
	docklet_update_status();
}
#endif

/**************************************************************************
 * docklet pop-up menu
 **************************************************************************/
static void
docklet_toggle_mute(GtkWidget *toggle, void *data)
{
	oul_prefs_set_bool(BEASY_PREFS_SND_MUTE, GTK_CHECK_MENU_ITEM(toggle)->active);
}

static void
docklet_plugin_actions(GtkWidget *menu)
{
	GtkWidget *menuitem, *submenu;
	OulPlugin *plugin = NULL;
	GList *l;
	int c = 0;

	g_return_if_fail(menu != NULL);

	/* Add a submenu for each plugin with custom actions */
	for (l = oul_plugins_get_loaded(); l; l = l->next) {
		plugin = (OulPlugin *) l->data;

		if (!OUL_PLUGIN_HAS_ACTIONS(plugin))
			continue;

		menuitem = gtk_image_menu_item_new_with_label(_(plugin->info->name));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		gtk_widget_show(menuitem);

		submenu = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), submenu);
		gtk_widget_show(submenu);

		build_plugin_actions(submenu, plugin, NULL);

		c++;
	}
	if(c>0)
		beasy_separator(menu);
}

static void
build_plugin_actions(GtkWidget *menu, OulPlugin *plugin, gpointer context)
{
	GtkWidget *menuitem;
	OulPluginAction *action = NULL;
	GList *actions, *l;

	actions = OUL_PLUGIN_ACTIONS(plugin, context);

	for (l = actions; l != NULL; l = l->next)
	{
		if (l->data)
		{
			action = (OulPluginAction *) l->data;
			action->plugin = plugin;
			action->context = context;

			menuitem = gtk_menu_item_new_with_label(action->label);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

			g_signal_connect(G_OBJECT(menuitem), "activate",
					G_CALLBACK(plugin_act_cb), action);
			g_object_set_data_full(G_OBJECT(menuitem), "plugin_action",
								   action,
								   (GDestroyNotify)oul_plugin_action_free);
			gtk_widget_show(menuitem);
		}
		else
			beasy_separator(menu);
	}

	g_list_free(actions);
}

static void
plugin_act_cb(GtkObject *obj, OulPluginAction *pam)
{
	if (pam && pam->callback)
		pam->callback(pam);
}

static void
docklet_menu(void)
{
	static GtkWidget *menu = NULL;
	GtkWidget *menuitem;

	if (menu) {
		gtk_widget_destroy(menu);
	}

	menu = gtk_menu_new();

	menuitem = gtk_menu_item_new_with_label(_("Unread Messages"));

	if (pending) {
		GtkWidget *submenu = gtk_menu_new();
		GList *l = get_pending_list(0);
		if (l == NULL) {
			gtk_widget_set_sensitive(menuitem, FALSE);
			oul_debug_warning("docklet",
				"status indicates messages pending, but no unseen messages were found.");
		} else {
			//beasy_conversations_fill_menu(submenu, l);
			g_list_free(l);
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), submenu);
		}
	} else {
		gtk_widget_set_sensitive(menuitem, FALSE);
	}
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	beasy_separator(menu);

	beasy_new_item_from_stock(menu, _("Plugins"), BEASY_STOCK_TOOLBAR_PLUGINS, G_CALLBACK(beasy_plugin_dialog_show), NULL, 0, 0, NULL);
	beasy_new_item_from_stock(menu, _("Preferences"), GTK_STOCK_PREFERENCES, G_CALLBACK(beasy_prefs_show), NULL, 0, 0, NULL);

	beasy_separator(menu);

	menuitem = gtk_check_menu_item_new_with_label(_("Mute Sounds"));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), oul_prefs_get_bool(BEASY_PREFS_SND_MUTE));

	gchar *soundmethod = oul_prefs_get_string(BEASY_PREFS_SND_METHOD);
	if (soundmethod && !strcmp(soundmethod, "none"))
		gtk_widget_set_sensitive(GTK_WIDGET(menuitem), FALSE);
	
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(docklet_toggle_mute), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	beasy_separator(menu);

	/* add plugin actions */
	docklet_plugin_actions(menu);

	beasy_new_item_from_stock(menu, _("Quit"), GTK_STOCK_QUIT, G_CALLBACK(oul_core_quit), NULL, 0, 0, NULL);

	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
		       ui_ops->position_menu,
		       NULL, 0, gtk_get_current_event_time());

}


/**************************************************************************
 * public api for ui_ops
 **************************************************************************/
void
beasy_docklet_update_icon()
{
	if (ui_ops && ui_ops->update_icon)
		ui_ops->update_icon(status, connecting, pending);
}

void
beasy_docklet_clicked(int button_type)
{
	switch (button_type) {
		case 1:
			if (pending) {
				GList *l = get_pending_list(1);
				if (l != NULL) {
					//beasy_conv_present_conversation((OulConversation *)l->data);
					g_list_free(l);
				}
			} else {
				//beasy_blist_toggle_visibility();
			}
			break;
		case 3:
			docklet_menu();
			break;
	}
}

void
beasy_docklet_embedded()
{
	visible = TRUE;
	docklet_update_status();
	beasy_docklet_update_icon();
}

void
beasy_docklet_remove()
{
	if (visible) {
		if (docklet_blinking_timer) {
			g_source_remove(docklet_blinking_timer);
			docklet_blinking_timer = 0;
		}
		visible = FALSE;
		status = OUL_STATUS_OFFLINE;
	}
}

void
beasy_docklet_set_ui_ops(struct docklet_ui_ops *ops)
{
	ui_ops = ops;
}

void*
beasy_docklet_get_handle()
{
	return &handle;
}

void
beasy_docklet_init()
{
	void *docklet_handle = beasy_docklet_get_handle();

	oul_prefs_add_none(BEASY_PREFS_ROOT "/docklet");
    
	docklet_ui_init();
	
	if(ui_ops && ui_ops->create)
		ui_ops->create();
}

void
beasy_docklet_uninit()
{
	if (visible && ui_ops && ui_ops->destroy)
		ui_ops->destroy();
}
