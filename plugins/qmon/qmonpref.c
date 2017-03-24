#include "internal.h"

#include "beasy.h"
#include "gtkprefs.h"
#include "plugin.h"
#include "gtkutils.h"
#include "qmonpref.h"


static const gchar *sr_selected = NULL;
static GtkListStore *srlist_store = NULL;
GtkWidget *remove_button = NULL;

static void
sr_list_refresh()
{
	GList *l, *ll = NULL;
	GtkTreeIter iter;

	l =	oul_prefs_get_string_list(PLUGIN_QMON_SRLIST);
	gtk_list_store_clear(srlist_store);
	for(ll = l;	ll; ll = ll->next) {
		gchar *sr_number = g_strdup((gchar *)ll->data);
		if(sr_number) {
			gtk_list_store_append (srlist_store, &iter);
			gtk_list_store_set(srlist_store, &iter, 0, sr_number, -1);

			g_free(sr_number);
		}
		g_free(ll->data);
	}
	g_list_free(l);
	
}

static void
sr_sel(GtkTreeSelection *sel, GtkTreeModel *model) 
{
	GtkTreeIter  iter;
	GValue val;

	if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
		/* Enable buttons if something is selected */
		gtk_widget_set_sensitive(remove_button, TRUE);

		val.g_type = 0;
		gtk_tree_model_get_value (model, &iter, 0, &val);
		sr_selected = g_value_get_string(&val);
	} else {
		/* Otherwise, disable them */
		gtk_widget_set_sensitive(remove_button, FALSE);
	}
	
}


static void
sr_add_cb(GtkWidget *button, gpointer data)
{
	GList *l;
	GtkEntry *entry = (GtkEntry *)data;

	gchar *sr_number =g_strstrip(g_strdup(gtk_entry_get_text(entry)));

	if(strlen(sr_number)  == 0)
		return;

	l = oul_prefs_get_string_list(PLUGIN_QMON_SRLIST);
	if(g_list_find_custom(l, sr_number, (GCompareFunc)strcasecmp))
		return;
	
	l = g_list_append(l, sr_number);
	gtk_entry_set_text(entry, "");
	
	oul_prefs_set_string_list(PLUGIN_QMON_SRLIST, l);
	g_list_free(l);

	sr_list_refresh();
}

static void
sr_remove_cb(GtkWidget *button, gpointer data)
{
	GList *l, *ll = NULL;

	if(!sr_selected)
		return;

	l = oul_prefs_get_string_list(PLUGIN_QMON_SRLIST);

	for(ll = l;	ll; ll = ll->next) {
		gchar *sr_number = (gchar *)ll->data;
		if(!strcasecmp(sr_number, sr_selected)) {
			l = g_list_remove(l, sr_number);
			g_free(sr_number);
		}
	}

	oul_prefs_set_string_list(PLUGIN_QMON_SRLIST, l);
	g_list_free(l);

	sr_selected = NULL;

	sr_list_refresh();
}

static void
sr_clear_cb()
{
	GList *l;

	oul_prefs_set_string_list(PLUGIN_QMON_SRLIST, NULL);
		
	sr_list_refresh();
}


static void
target_changed1_cb(const char *name, OulPrefType type, gconstpointer value, gpointer data)
{
	GtkWidget *hbox = data;
	int target = GPOINTER_TO_INT(value);

	if(target == QMON_TARGET_ANALYST)
		gtk_widget_set_sensitive(hbox, TRUE);
	else
		gtk_widget_set_sensitive(hbox, FALSE);

}

static void
target_changed2_cb(const char *name, OulPrefType type, gconstpointer value, gpointer data)
{
	GtkWidget *hbox = data;
	int target = GPOINTER_TO_INT(value);

	if(target == QMON_TARGET_SRLIST)
		gtk_widget_set_sensitive(hbox, TRUE);
	else
		gtk_widget_set_sensitive(hbox, FALSE);

}

static gint
analyst_name_changed_cb(GtkEntry *entry, gpointer data)
{
	oul_prefs_set_string(PLUGIN_QMON_ANALYST, gtk_entry_get_text(GTK_ENTRY(entry)));
	return TRUE;
}

static void
username_changed_cb(GtkEntry *entry, gpointer data)
{
	oul_prefs_set_string(PLUGIN_QMON_USERNAME, gtk_entry_get_text(GTK_ENTRY(entry)));
}

static void
password_changed_cb(GtkEntry *entry, gpointer data)
{
	oul_prefs_set_string(PLUGIN_QMON_PASSWORD, gtk_entry_get_text(GTK_ENTRY(entry)));
}

static void
selection_changed_cb(GtkEntry *entry, gpointer data)
{
	oul_prefs_set_string(PLUGIN_QMON_SELECTION, gtk_entry_get_text(GTK_ENTRY(entry)));
}

static int
notebook_add_page(GtkWidget *notebook, const char *text, GtkWidget *page)
{

#if GTK_CHECK_VERSION(2,4,0)
	return gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, gtk_label_new(text));
#else
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, gtk_label_new(text));
	return gtk_notebook_page_num(GTK_NOTEBOOK(notebook), page);
#endif
}


static GtkWidget *
page_qmon_logininfo_get()
{
	GtkWidget *ret;
	GtkWidget *vbox, *hbox, *entry, *label;
	GtkSizeGroup *sg;

	ret = gtk_vbox_new(FALSE, BEASY_HIG_CAT_SPACE);
	gtk_container_set_border_width (GTK_CONTAINER(ret), BEASY_HIG_BORDER);
	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	vbox = beasy_make_frame(ret, _("Login Information"));
	gtk_container_set_border_width(GTK_CONTAINER (ret), BEASY_HIG_BORDER);

	/* username */
	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), oul_prefs_get_string(PLUGIN_QMON_USERNAME));
	hbox = beasy_add_widget_to_vbox(GTK_BOX(vbox), _("Qmon Username(e.g: RFAN):"),
									sg, entry, TRUE, NULL);
	label = gtk_label_new("*");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 1);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(username_changed_cb), NULL);

	/* password */
	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), oul_prefs_get_string(PLUGIN_QMON_PASSWORD));
	gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
	if (gtk_entry_get_invisible_char(GTK_ENTRY(entry)) == '*')
		gtk_entry_set_invisible_char(GTK_ENTRY(entry), BEASY_INVISIBLE_CHAR);
	
	hbox = beasy_add_widget_to_vbox(GTK_BOX(vbox), _("Qmon Password:"),
									sg, entry, TRUE, NULL);
	label = gtk_label_new("*");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 1);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(password_changed_cb), NULL);

	/* qmon selection */
	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), oul_prefs_get_string(PLUGIN_QMON_SELECTION));
	hbox = beasy_add_widget_to_vbox(GTK_BOX(vbox), _("Qmon Selection(e.g: 1309 and 4455):"),
									sg, entry, TRUE, NULL);
	label = gtk_label_new("*");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 1);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(selection_changed_cb), NULL);

	
	g_object_unref(sg);
	gtk_widget_show_all(ret);

	return ret;
}


static GtkWidget *
page_monitor_details_get()
{
	GtkWidget *ret, *dd;
	GtkWidget *vbox, *hbox;
	GtkSizeGroup *sg;
	GtkWidget *entry, *button, *sw;
	GtkTreeIter iter;

	GtkWidget *srlist_view;
	GtkCellRenderer *rend;
	GtkTreeViewColumn *col;
	GtkTreeSelection *sel;
	
	ret = gtk_vbox_new(FALSE, BEASY_HIG_CAT_SPACE);
	gtk_container_set_border_width (GTK_CONTAINER(ret), BEASY_HIG_BORDER);

	/* general setting */
	vbox = beasy_make_frame(ret, _("General Setting"));
	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	dd	= beasy_prefs_dropdown(vbox, _("_Method:"), OUL_PREF_INT, PLUGIN_QMON_TARGET, 
			_("Monitor As CTC"),	QMON_TARGET_CTC,
			_("Monitor Analyst"), 	QMON_TARGET_ANALYST,
			_("Monitor SR List"),	QMON_TARGET_SRLIST, NULL);

	beasy_prefs_labeled_spin_button(vbox, _("Interval:"), PLUGIN_QMON_INTERVAL, 1, 60, sg);
	g_object_unref(sg);

	/* analyst monitor parameters */
	vbox = beasy_make_frame (ret, _("Analyst Monitor Setting"));
	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	
	entry = gtk_entry_new();
	hbox = beasy_add_widget_to_vbox(GTK_BOX(vbox), _("Analyst Global Name(e.g: RFAN):"),
									sg, entry, TRUE, NULL);
	
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(analyst_name_changed_cb), NULL);
	gtk_widget_set_sensitive(hbox,
		(oul_prefs_get_int(PLUGIN_QMON_TARGET) == QMON_TARGET_ANALYST));
	oul_prefs_connect_callback(NULL, PLUGIN_QMON_TARGET, target_changed1_cb, hbox);

	/*SR list monitor parameters */
	vbox = beasy_make_frame (ret, _("SRList Monitor Setting"));

	/* The following is an ugly hack to make the frame expand so the
	 * sound events list is big enough to be usable */
	gtk_box_set_child_packing(GTK_BOX(vbox->parent), vbox, TRUE, TRUE, 0,
			GTK_PACK_START);
	gtk_box_set_child_packing(GTK_BOX(vbox->parent->parent), vbox->parent, TRUE,
			TRUE, 0, GTK_PACK_START);
	gtk_box_set_child_packing(GTK_BOX(vbox->parent->parent->parent),
			vbox->parent->parent, TRUE, TRUE, 0, GTK_PACK_START);

	sw = gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_set_size_request(sw, -1, 150);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_IN);

	gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

	srlist_store = gtk_list_store_new(1, G_TYPE_STRING);
	srlist_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(srlist_store));
	g_object_unref(G_OBJECT(srlist_store));

	sr_list_refresh();

	rend = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("SR Number"), rend,
							"text", 0,
							NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(srlist_view), col);
	
	gtk_container_add(GTK_CONTAINER(sw), srlist_view);	

	hbox = gtk_hbox_new(FALSE, BEASY_HIG_BOX_SPACE);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	
	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, BEASY_HIG_BOX_SPACE);

	button = gtk_button_new_with_mnemonic(_("_Add"));
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(sr_add_cb), entry);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 1);

	
	remove_button = gtk_button_new_with_mnemonic(_("_Remove"));
	g_signal_connect(G_OBJECT(remove_button), "clicked", G_CALLBACK(sr_remove_cb), NULL);
	gtk_widget_set_sensitive(remove_button, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), remove_button, FALSE, FALSE, 1);

	button = gtk_button_new_with_mnemonic(_("_Clear All"));
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(sr_clear_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 1);

	gtk_widget_set_sensitive(hbox, 
				(oul_prefs_get_int(PLUGIN_QMON_TARGET) == QMON_TARGET_SRLIST));
	oul_prefs_connect_callback(NULL, PLUGIN_QMON_TARGET, target_changed2_cb, hbox);

	/* Force the selection mode */
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (srlist_view));
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(sr_sel), NULL);

	g_object_unref(sg);
	gtk_widget_show_all(ret);


	return ret;
}

GtkWidget *
qmon_prefs_frame_get(OulPlugin *plugin)
{
	GtkWidget *notebook;

	notebook = gtk_notebook_new();
	gtk_widget_show(notebook);

	notebook_add_page(notebook, "QmonLogin", 	page_qmon_logininfo_get());
	notebook_add_page(notebook, "Monitor",		page_monitor_details_get());

	return notebook;
}

void
qmon_prefs_init()
{
	oul_prefs_add_none(PLUGIN_QMON_ROOT);
	oul_prefs_add_string(PLUGIN_QMON_USERNAME, "");
	oul_prefs_add_string(PLUGIN_QMON_PASSWORD, "");
	oul_prefs_add_string(PLUGIN_QMON_SELECTION, "");


	/* interval unit is minutes */
	oul_prefs_add_int(PLUGIN_QMON_INTERVAL, 5);

	oul_prefs_add_int(PLUGIN_QMON_TARGET, QMON_TARGET_CTC);
	oul_prefs_add_string(PLUGIN_QMON_ANALYST, "");
	oul_prefs_add_string_list(PLUGIN_QMON_SRLIST, NULL);

}


