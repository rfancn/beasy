#include "internal.h"

#include "prefs.h"

#include "gtkutils.h"
#include "gtkprefs.h"
#include "gtksound.h"
#include "gtkpref_sound.h"


static int 			sound_row_sel = 0;
static GtkWidget 	*sound_entry = NULL;

static void
event_toggled(GtkCellRendererToggle *cell, gchar *pth, gpointer data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreeIter iter;
	GtkTreePath *path = gtk_tree_path_new_from_string(pth);
	char *pref;

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter,
						2, &pref,
						-1);

	oul_prefs_set_bool(pref, !gtk_cell_renderer_toggle_get_active(cell));
	g_free(pref);

	gtk_list_store_set(GTK_LIST_STORE (model), &iter,
					   0, !gtk_cell_renderer_toggle_get_active(cell),
					   -1);

	gtk_tree_path_free(path);
}


static void
test_sound(GtkWidget *button, gpointer i_am_NULL)
{
	char *pref;
	gboolean temp_enabled;
	gboolean temp_mute;

	pref = g_strdup_printf(BEASY_PREFS_SND_ENABLED_ROOT "/%s",
			beasy_sound_get_event_option(sound_row_sel));

	temp_enabled = oul_prefs_get_bool(pref);
	temp_mute = oul_prefs_get_bool(BEASY_PREFS_SND_MUTE);

	if (!temp_enabled) oul_prefs_set_bool(pref, TRUE);
	if (temp_mute) oul_prefs_set_bool(BEASY_PREFS_SND_MUTE, FALSE);

	oul_sound_play_event(sound_row_sel);

	if (!temp_enabled) oul_prefs_set_bool(pref, FALSE);
	if (temp_mute) oul_prefs_set_bool(BEASY_PREFS_SND_MUTE, TRUE);

	g_free(pref);
}

/*
 * Resets a sound file back to default.
 */
static void
reset_sound(GtkWidget *button, gpointer i_am_also_NULL)
{
	gchar *pref;

	pref = g_strdup_printf(BEASY_PREFS_SND_FILE_ROOT "/%s",
						   beasy_sound_get_event_option(sound_row_sel));
	oul_prefs_set_path(pref, "");
	g_free(pref);

	gtk_entry_set_text(GTK_ENTRY(sound_entry), _("(default)"));
}

static void
sound_chosen_cb(void *user_data, const char *filename)
{
	gchar *pref;
	int sound;

	sound = GPOINTER_TO_INT(user_data);

	/* Set it -- and forget it */
	pref = g_strdup_printf(BEASY_PREFS_SND_FILE_ROOT "/%s",
						   beasy_sound_get_event_option(sound));
	oul_prefs_set_path(pref, filename);
	g_free(pref);

	/*
	 * If the sound we just changed is still the currently selected
	 * sound, then update the box showing the file name.
	 */
	if (sound == sound_row_sel)
		gtk_entry_set_text(GTK_ENTRY(sound_entry), filename);
}


static void
prefs_sound_sel(GtkTreeSelection *sel, GtkTreeModel *model) {
	GtkTreeIter  iter;
	GValue val;
	const char *file;
	char *pref;

	if (! gtk_tree_selection_get_selected (sel, &model, &iter))
		return;

	val.g_type = 0;
	gtk_tree_model_get_value (model, &iter, 3, &val);
	sound_row_sel = g_value_get_uint(&val);

	pref = g_strdup_printf(BEASY_PREFS_SND_FILE_ROOT "/%s",
			beasy_sound_get_event_option(sound_row_sel));
	file = oul_prefs_get_path(pref);
	g_free(pref);
	if (sound_entry)
		gtk_entry_set_text(GTK_ENTRY(sound_entry), (file && *file != '\0') ? file : _("(default)"));
	g_value_unset (&val);
}


static void
sound_changed1_cb(const char *name, OulPrefType type,
				  gconstpointer value, gpointer data)
{
	GtkWidget *hbox = data;
	const char *method = value;

	gtk_widget_set_sensitive(hbox, !strcmp(method, "custom"));
}


static void
sound_changed2_cb(const char *name, OulPrefType type,
				  gconstpointer value, gpointer data)
{
	GtkWidget *vbox = data;
	const char *method = value;

	gtk_widget_set_sensitive(vbox, strcmp(method, "none"));
}


#ifdef USE_GSTREAMER
static gchar* prefs_sound_volume_format(GtkScale *scale, gdouble val)
{
	if(val < 15) {
		return g_strdup_printf(_("Quietest"));
	} else if(val < 30) {
		return g_strdup_printf(_("Quieter"));
	} else if(val < 45) {
		return g_strdup_printf(_("Quiet"));
	} else if(val < 55) {
		return g_strdup_printf(_("Normal"));
	} else if(val < 70) {
		return g_strdup_printf(_("Loud"));
	} else if(val < 85) {
		return g_strdup_printf(_("Louder"));
	} else {
		return g_strdup_printf(_("Loudest"));
	}
}

static void prefs_sound_volume_changed(GtkRange *range)
{
	int val = (int)gtk_range_get_value(GTK_RANGE(range));
	oul_prefs_set_int(BEASY_PREFS_SND_VOLUMN, val);
}

static void
sound_changed3_cb(const char *name, OulPrefType type,
				  gconstpointer value, gpointer data)
{
	GtkWidget *hbox = data;
	const char *method = value;

	gtk_widget_set_sensitive(hbox,
			!strcmp(method, "automatic") ||
			!strcmp(method, "esd"));
}
#endif /* USE_GSTREAMER */


static void
select_sound(GtkWidget *button, gpointer being_NULL_is_fun)
{
	gchar *pref;
	const char *filename;

	pref = g_strdup_printf(BEASY_PREFS_SND_FILE_ROOT "/%s",
						   beasy_sound_get_event_option(sound_row_sel));
	filename = oul_prefs_get_path(pref);
	g_free(pref);

	if (*filename == '\0')
		filename = NULL;

	/**
	oul_request_file(prefs, _("Sound Selection"), filename, FALSE,
					  G_CALLBACK(sound_chosen_cb), NULL,
					  NULL, NULL, NULL,
					  GINT_TO_POINTER(sound_row_sel));
	**/
}

static gint
sound_cmd_yeah(GtkEntry *entry, gpointer d)
{
	oul_prefs_set_path(BEASY_PREFS_SND_COMMAND, gtk_entry_get_text(GTK_ENTRY(entry)));
	return TRUE;
}

GtkWidget *
beasy_sound_prefpage_get(void)
{
	GtkWidget *ret;
	GtkWidget *vbox, *sw, *button;
	GtkSizeGroup *sg;
	GtkTreeIter iter;
	GtkWidget *event_view;
	GtkListStore *event_store;
	GtkCellRenderer *rend;
	GtkTreeViewColumn *col;
	GtkTreeSelection *sel;
	GtkTreePath *path;
	GtkWidget *hbox;
	int j;
	const char *file;
	char *pref;
	GtkWidget *dd;
	GtkWidget *entry;
	const char *cmd;

	ret = gtk_vbox_new(FALSE, BEASY_HIG_CAT_SPACE);
	gtk_container_set_border_width (GTK_CONTAINER (ret), BEASY_HIG_BORDER);

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	vbox = beasy_make_frame (ret, _("Sound Method"));
	dd = beasy_prefs_dropdown(vbox, _("_Method:"), OUL_PREF_STRING, BEASY_PREFS_SND_METHOD, 
			_("Console beep"), "beep",
#ifdef USE_GSTREAMER
			_("Automatic"), "automatic",
			"ESD", 			"esd",
			"ALSA", 		"alsa",
#endif
			_("Command"), 	"custom",
			_("No sounds"), "none",
			NULL);

	gtk_size_group_add_widget(sg, dd);
	gtk_misc_set_alignment(GTK_MISC(dd), 0, 0.5);

	entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
	cmd = oul_prefs_get_path(BEASY_PREFS_SND_COMMAND);
	if(cmd)
		gtk_entry_set_text(GTK_ENTRY(entry), cmd);
	
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(sound_cmd_yeah), NULL);

	hbox = beasy_add_widget_to_vbox(GTK_BOX(vbox), _("Sound c_ommand:\n(%s for filename)"), sg, entry, TRUE, NULL);
	oul_prefs_connect_callback(beasy_prefs_get(), BEASY_PREFS_SND_METHOD, sound_changed1_cb, hbox);
	gtk_widget_set_sensitive(hbox, !strcmp(oul_prefs_get_string(BEASY_PREFS_SND_METHOD), "custom"));

#ifdef USE_GSTREAMER
	sw = gtk_hscale_new_with_range(0.0, 100.0, 5.0);
	gtk_range_set_increments(GTK_RANGE(sw), 5.0, 25.0);
	gtk_range_set_value(GTK_RANGE(sw), oul_prefs_get_int(BEASY_PREFS_SND_VOLUMN));
	g_signal_connect (G_OBJECT (sw), "format-value",
			  G_CALLBACK (prefs_sound_volume_format),
			  NULL);
	g_signal_connect (G_OBJECT (sw), "value-changed",
			  G_CALLBACK (prefs_sound_volume_changed),
			  NULL);
	hbox = beasy_add_widget_to_vbox(GTK_BOX(vbox), _("Volume:"), NULL, sw, TRUE, NULL);

	oul_prefs_connect_callback(beasy_prefs_get(), BEASY_PREFS_SND_METHOD,
								sound_changed3_cb, hbox);
	sound_changed3_cb(BEASY_PREFS_SND_METHOD, OUL_PREF_STRING,
			  oul_prefs_get_string(BEASY_PREFS_SND_METHOD), hbox);
#endif

	gtk_widget_set_sensitive(vbox,
			strcmp(oul_prefs_get_string(BEASY_PREFS_SND_METHOD), "none"));
	oul_prefs_connect_callback(beasy_prefs_get(), BEASY_PREFS_SND_METHOD,
								sound_changed2_cb, vbox);

	vbox = beasy_make_frame(ret, _("Sound Events"));

	/* The following is an ugly hack to make the frame expand so the
	 * sound events list is big enough to be usable */
	gtk_box_set_child_packing(GTK_BOX(vbox->parent), vbox, TRUE, TRUE, 0,
			GTK_PACK_START);
	gtk_box_set_child_packing(GTK_BOX(vbox->parent->parent), vbox->parent, TRUE,
			TRUE, 0, GTK_PACK_START);
	gtk_box_set_child_packing(GTK_BOX(vbox->parent->parent->parent),
			vbox->parent->parent, TRUE, TRUE, 0, GTK_PACK_START);

	sw = gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_set_size_request(sw, -1, 100);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_IN);

	gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
	event_store = gtk_list_store_new (4, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT);

	for (j=0; j < OUL_NUM_SOUNDS; j++) {
		char *pref = g_strdup_printf(BEASY_PREFS_SND_ENABLED_ROOT "/%s", 
									beasy_sound_get_event_option(j));
		
		const char *label = beasy_sound_get_event_label(j);

		if (label == NULL) {
			g_free(pref);
			continue;
		}

		gtk_list_store_append (event_store, &iter);
		gtk_list_store_set(event_store, &iter,
				   0, oul_prefs_get_bool(pref),
				   1, _(label),
				   2, pref,
				   3, j,
				   -1);
		g_free(pref);
	}

	event_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL(event_store));

	rend = gtk_cell_renderer_toggle_new();
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (event_view));
	g_signal_connect (G_OBJECT (sel), "changed", G_CALLBACK (prefs_sound_sel), NULL);
	g_signal_connect (G_OBJECT(rend), "toggled", G_CALLBACK(event_toggled), event_store);
	path = gtk_tree_path_new_first();
	gtk_tree_selection_select_path(sel, path);
	gtk_tree_path_free(path);

	col = gtk_tree_view_column_new_with_attributes (_("Play"), rend, "active", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(event_view), col);

	rend = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Event"),
							rend,
							"text", 1,
							NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(event_view), col);
	g_object_unref(G_OBJECT(event_store));
	gtk_container_add(GTK_CONTAINER(sw), event_view);

	hbox = gtk_hbox_new(FALSE, BEASY_HIG_BOX_SPACE);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	sound_entry = gtk_entry_new();
	pref = g_strdup_printf(BEASY_PREFS_SND_FILE_ROOT "/%s",
			       beasy_sound_get_event_option(0));
	file = oul_prefs_get_path(pref);
	g_free(pref);
	gtk_entry_set_text(GTK_ENTRY(sound_entry), (file && *file != '\0') ? file : _("(default)"));
	gtk_editable_set_editable(GTK_EDITABLE(sound_entry), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), sound_entry, FALSE, FALSE, BEASY_HIG_BOX_SPACE);

	button = gtk_button_new_with_label(_("Test"));
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(test_sound), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 1);

	button = gtk_button_new_with_label(_("Reset"));
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(reset_sound), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 1);

	button = gtk_button_new_with_label(_("Choose..."));
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(select_sound), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 1);

	gtk_widget_show_all(ret);
	g_object_unref(sg);

	return ret;
}

void
beasy_sndntf_prefpage_destroy()
{
	sound_entry = NULL;
}

void
beasy_sound_prefs_init(void)
{
	oul_prefs_add_none(BEASY_PREFS_SND_ROOT);

	oul_prefs_add_none(BEASY_PREFS_SND_ENABLED_ROOT);
	oul_prefs_add_bool(BEASY_PREFS_SND_ENABLED_INFO, 	TRUE);
	oul_prefs_add_bool(BEASY_PREFS_SND_ENABLED_WARN,	TRUE);
	oul_prefs_add_bool(BEASY_PREFS_SND_ENABLED_ERROR,	TRUE);
	oul_prefs_add_bool(BEASY_PREFS_SND_ENABLED_FATAL,	TRUE);

	oul_prefs_add_none(BEASY_PREFS_SND_FILE_ROOT);
	oul_prefs_add_path(BEASY_PREFS_SND_FILE_INFO,	"");
	oul_prefs_add_path(BEASY_PREFS_SND_FILE_WARN, 	"");
	oul_prefs_add_path(BEASY_PREFS_SND_FILE_ERROR,	"");
	oul_prefs_add_path(BEASY_PREFS_SND_FILE_FATAL,	"");

	oul_prefs_add_string(BEASY_PREFS_SND_METHOD,	"automatic");	
	oul_prefs_add_path(BEASY_PREFS_SND_COMMAND,	"");
	oul_prefs_add_int(BEASY_PREFS_SND_VOLUMN,		50);
	oul_prefs_add_bool(BEASY_PREFS_SND_MUTE,		FALSE);
}

