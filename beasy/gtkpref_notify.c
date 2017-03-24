#include "internal.h"

#include "beasy.h"
#include "gtkutils.h"
#include "gtkprefs.h"

#include "gtkntf_display.h"
#include "gtkntf_event.h"
#include "gtkpref_notify.h"

enum {
	GTKNTF_NOTIF_COL_SHOW = 0,
	GTKNTF_NOTIF_COL_NAME,
	GTKNTF_NOTIF_COL_DESCRIPTION,
	GTKNTF_NOTIF_COL_TYPE
};

static void
notification_show_cb(GtkCellRendererToggle *renderer, gchar *path,
					 gpointer data)
{
	GtkNtfEvent *event;
	GtkListStore *store = (GtkListStore *)data;
	GtkTreeIter iter;
	gchar *type = NULL;
	gboolean show = FALSE;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
					   GTKNTF_NOTIF_COL_SHOW, &show,
					   GTKNTF_NOTIF_COL_TYPE, &type,
					   -1);

	event = gtkntf_event_find_for_notification(type);
	if(event) {
		gtkntf_event_set_show(event, !show);
		gtkntf_events_save();
	}

	g_free(type);

	gtk_list_store_set(store, &iter,
					   GTKNTF_NOTIF_COL_SHOW, !show,
					   -1);
}



static gint
notification_sort_desc(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,
					   gpointer data)
{
	gchar *a_desc = NULL, *b_desc = NULL;
	gchar *a_ckey = NULL, *b_ckey = NULL;
	gint ret = 0;

	gtk_tree_model_get(model, a, GTKNTF_NOTIF_COL_DESCRIPTION, &a_desc, -1);
	gtk_tree_model_get(model, b, GTKNTF_NOTIF_COL_DESCRIPTION, &b_desc, -1);

	a_ckey = g_utf8_collate_key(a_desc, g_utf8_strlen(a_desc, -1));
	b_ckey = g_utf8_collate_key(b_desc, g_utf8_strlen(b_desc, -1));

	g_free(a_desc);
	g_free(b_desc);

	ret = strcmp(a_ckey, b_ckey);

	g_free(a_ckey);
	g_free(b_ckey);

	return ret;
}


static gint
notification_sort_name(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,
					   gpointer data)
{
	gchar *a_name = NULL, *b_name = NULL;
	gchar *a_ckey = NULL, *b_ckey = NULL;
	gint ret = 0;

	gtk_tree_model_get(model, a, GTKNTF_NOTIF_COL_NAME, &a_name, -1);
	gtk_tree_model_get(model, b, GTKNTF_NOTIF_COL_NAME, &b_name, -1);

	a_ckey = g_utf8_collate_key(a_name, g_utf8_strlen(a_name, -1));
	b_ckey = g_utf8_collate_key(b_name, g_utf8_strlen(b_name, -1));

	g_free(a_name);
	g_free(b_name);

	ret = strcmp(a_ckey, b_ckey);

	g_free(a_ckey);
	g_free(b_ckey);

	return ret;
}


static gint
notification_sort_show(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,
					   gpointer data)
{
	gboolean a_show = FALSE, b_show = FALSE;

	gtk_tree_model_get(model, a, GTKNTF_NOTIF_COL_SHOW, &a_show, -1);
	gtk_tree_model_get(model, b, GTKNTF_NOTIF_COL_SHOW, &b_show, -1);

	if(a_show && !b_show)
		return 1;
	else if(!a_show && b_show)
		return -1;
	else
		return 0;
}


static void
make_notification_list(GtkBox *parent) {
	GtkWidget *list, *sw;
	GtkListStore *store;
	GtkTreeSortable *sortable;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;
	const GList *events;

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
								   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(parent, sw, TRUE, TRUE, 0);

	store = gtk_list_store_new(4, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING,
							   G_TYPE_STRING);

	for(events = gtkntf_events_get(); events; events = events->next) {
		GtkNtfEvent *event;
		GtkTreeIter iter;
		const gchar *type;

		event = GTKNTF_EVENT(events->data);
		type = 	gtkntf_event_get_notification_type(event);

		if(type && type[0] == '!')
			continue;

		gtk_list_store_append(store, &iter);

		gtk_list_store_set(store, &iter,
						   GTKNTF_NOTIF_COL_SHOW, gtkntf_event_show_notification(type),
						   GTKNTF_NOTIF_COL_NAME, gtkntf_event_get_name(event),
						   GTKNTF_NOTIF_COL_DESCRIPTION, gtkntf_event_get_description(event),
						   GTKNTF_NOTIF_COL_TYPE, type,
						   -1);
	}

	sortable = GTK_TREE_SORTABLE(store);
	gtk_tree_sortable_set_sort_func(sortable, GTKNTF_NOTIF_COL_SHOW,
									notification_sort_show, NULL, NULL);
	gtk_tree_sortable_set_sort_func(sortable, GTKNTF_NOTIF_COL_NAME,
									notification_sort_name, NULL, NULL);
	gtk_tree_sortable_set_sort_func(sortable, GTKNTF_NOTIF_COL_DESCRIPTION,
									notification_sort_desc, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(sortable, GTKNTF_NOTIF_COL_NAME,
										 GTK_SORT_ASCENDING);

	list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(list), TRUE);
	gtk_widget_set_size_request(list, -1, 150);
	gtk_container_add(GTK_CONTAINER(sw), list);

	renderer = gtk_cell_renderer_toggle_new();
	g_signal_connect(G_OBJECT(renderer), "toggled",
					 G_CALLBACK(notification_show_cb), store);
	
	col = gtk_tree_view_column_new_with_attributes(_("Show"), renderer, "active", 0, NULL);
	
	gtk_tree_view_column_set_sort_column_id(col, GTKNTF_NOTIF_COL_SHOW);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Event"), renderer,
												   "text", 1, NULL);
	gtk_tree_view_column_set_sort_column_id(col, GTKNTF_NOTIF_COL_NAME);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Description"), renderer,
												   "text", 2, NULL);
	gtk_tree_view_column_set_sort_column_id(col, GTKNTF_NOTIF_COL_DESCRIPTION);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), col);

	gtk_widget_show_all(sw);
}

static GtkWidget *
make_label(const gchar *text, GtkSizeGroup *sg) {
	GtkWidget *label;

	label = gtk_label_new_with_mnemonic(text);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);

	if(sg)
		gtk_size_group_add_widget(sg, label);

	return label;
}


static void
make_display_options(GtkWidget *parent, GtkSizeGroup *sg)
{
	GtkWidget *frame, *spin, *option, *label;
	
	g_return_if_fail(parent != NULL);

	/* Display Options */
	frame = beasy_make_frame(parent, _("Display Options"));
	gtk_widget_show(frame);

	option = beasy_prefs_dropdown(frame, _("_Position:"), OUL_PREF_INT, BEASY_PREFS_NTF_APPEARANCE_POSITION,
								_("Top Left"), GTKNTF_DISPLAY_POSITION_NW,
								_("Top Right"), GTKNTF_DISPLAY_POSITION_NE,
								_("Bottom Left"), GTKNTF_DISPLAY_POSITION_SW,
								_("Bottom Right"), GTKNTF_DISPLAY_POSITION_SE,
								NULL);

	gtk_misc_set_alignment(GTK_MISC(option), 0, 0);
	gtk_size_group_add_widget(sg, option);
	

	label = beasy_prefs_dropdown(frame, _("_Stack:"), OUL_PREF_BOOLEAN, BEASY_PREFS_NTF_APPEARANCE_VERTICAL,
									_("Vertically"), TRUE,
									_("Horizontally"), FALSE,
									NULL);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_size_group_add_widget(sg, label);

	label = beasy_prefs_dropdown(frame, _("_Animate:"), OUL_PREF_BOOLEAN,
									BEASY_PREFS_NTF_APPEARANCE_ANIMATE,
									_("Yes"), TRUE,
									_("No"), FALSE,
									NULL);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_size_group_add_widget(sg, label);

	spin = beasy_prefs_labeled_spin_button(frame, _("_Display Time:"),
											  BEASY_PREFS_NTF_BEHAVIOR_DISPLAY_TIME,
											  1, 60, sg);
	label = make_label(_("seconds"), NULL);
	gtk_box_pack_start(GTK_BOX(spin), label, FALSE, FALSE, 0);
	
}

void
make_mouse_options(GtkWidget *parent, GtkSizeGroup *sg)
{
	GtkWidget *frame, *option;
	
	/* Mouse Options */
	frame = beasy_make_frame(parent, _("Mouse"));
	gtk_widget_show(frame);

	option = beasy_prefs_dropdown(frame, _("_Left:"), OUL_PREF_STRING, BEASY_PREFS_NTF_MOUSE_LEFT,
									_("Get Info"), 	"info",
									_("Close"), 	"close",
									NULL);
	
	gtk_misc_set_alignment(GTK_MISC(option), 0, 0);
	gtk_size_group_add_widget(sg, option);


	option = beasy_prefs_dropdown(frame, _("_Right:"), OUL_PREF_STRING, BEASY_PREFS_NTF_MOUSE_RIGHT,
									_("Get Info"), 	"info",
									_("Close"), 	"close",
									NULL);
	
	gtk_misc_set_alignment(GTK_MISC(option), 0, 0);
	gtk_size_group_add_widget(sg, option);

}


GtkWidget *
beasy_notify_prefpage_get(void)
{
	GtkWidget *ret, *frame, *option, *spin, *label;
	GtkSizeGroup *sg;

	ret = gtk_vbox_new(FALSE, BEASY_HIG_CAT_SPACE);
	gtk_container_set_border_width (GTK_CONTAINER (ret), BEASY_HIG_BORDER);

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	/* Display Options */
	make_display_options(ret, sg);
	make_mouse_options(ret, sg);
	make_notification_list(GTK_BOX(ret));
	
	gtk_widget_show_all(ret);
	g_object_unref(sg);

	return ret;
}

void
beasy_notify_prefs_init() {
	GList *l = NULL;
	gchar *def_theme = NULL;

	oul_prefs_add_none(BEASY_PREFS_NTF_ROOT);

	oul_prefs_add_none(BEASY_PREFS_NTF_BEHAVIOR_ROOT);
	oul_prefs_add_int(BEASY_PREFS_NTF_BEHAVIOR_DISPLAY_TIME, 6);
	oul_prefs_add_int(BEASY_PREFS_NTF_BEHAVIOR_THROTTLE, 6);

	oul_prefs_add_none(BEASY_PREFS_NTF_APPEARANCE_ROOT);
	oul_prefs_add_int(BEASY_PREFS_NTF_APPEARANCE_POSITION, GTKNTF_DISPLAY_POSITION_SE);
	oul_prefs_add_bool(BEASY_PREFS_NTF_APPEARANCE_VERTICAL, TRUE);
	oul_prefs_add_bool(BEASY_PREFS_NTF_APPEARANCE_ANIMATE, TRUE);

	oul_prefs_add_none(BEASY_PREFS_NTF_MOUSE_ROOT);
	oul_prefs_add_string(BEASY_PREFS_NTF_MOUSE_LEFT, "info");
	oul_prefs_add_string(BEASY_PREFS_NTF_MOUSE_RIGHT, "close");

	def_theme = g_build_filename(PKGDATADIR, "themes", "default", "theme.xml", NULL);
	
	l = g_list_append(l, def_theme);
	oul_prefs_add_string_list(BEASY_PREFS_NTF_LOADED_THEMES, l);
	g_free(def_theme);
	g_list_free(l);

	oul_prefs_add_none(BEASY_PREFS_NTF_ADVANCED_ROOT);

#if GTK_CHECK_VERSION(2,2,0)
	oul_prefs_add_int(BEASY_PREFS_NTF_ADVANCED_SCREEN, 0);
	oul_prefs_add_int(BEASY_PREFS_NTF_ADVANCED_MONITOR, 0);

	if(oul_prefs_get_int(BEASY_PREFS_NTF_ADVANCED_SCREEN) >
	   gtkntf_display_get_screen_count())
	{
		oul_prefs_set_int(BEASY_PREFS_NTF_ADVANCED_SCREEN,
						   gtkntf_display_get_default_screen());
	}

	if(oul_prefs_get_int(BEASY_PREFS_NTF_ADVANCED_MONITOR) >
	   gtkntf_display_get_monitor_count())
	{
		oul_prefs_set_int(BEASY_PREFS_NTF_ADVANCED_MONITOR,
						   gtkntf_display_get_default_monitor());
	}
#endif /* GTK_CHECK_VERSION(2,2,0) */

}

