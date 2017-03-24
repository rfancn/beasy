#include "internal.h"

#include "prefs.h"

#include "beasy.h"
#include "gtkutils.h"
#include "gtkprefs.h"


#include "gtkpref_notify.h"
#include "gtkpref_sound.h"
#include "gtkpref_net.h"
#include "gtkpref_plugin.h"

static GtkWidget 	*prefs 			= NULL;
static int 			notebook_page 	= 0;
static GtkWidget 	*prefsnotebook;

static int
prefs_notebook_add_page(const char *text, GtkWidget *page, int ind)
{

#if GTK_CHECK_VERSION(2,4,0)
	return gtk_notebook_append_page(GTK_NOTEBOOK(prefsnotebook), page, gtk_label_new(text));
#else
	gtk_notebook_append_page(GTK_NOTEBOOK(prefsnotebook), page, gtk_label_new(text));
	return gtk_notebook_page_num(GTK_NOTEBOOK(prefsnotebook), page);
#endif
}

static void
entry_set(GtkEntry *entry, gpointer data) {
	const char *key = (const char*)data;

	oul_prefs_set_string(key, gtk_entry_get_text(entry));
}


GtkWidget *
beasy_prefs_labeled_entry(GtkWidget *page, const gchar *title,
							 const char *key, GtkSizeGroup *sg)
{
	GtkWidget *entry;
	const gchar *value;

	value = oul_prefs_get_string(key);

	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), value);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(entry_set), (gchar *)key);
	gtk_widget_show(entry);

	return beasy_add_widget_to_vbox(GTK_BOX(page), title, sg, entry, TRUE, NULL);
}

static void
set_bool_pref(GtkWidget *w, const char *key)
{
	oul_prefs_set_bool(key,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
}



static void
update_spin_value(GtkWidget *w, GtkWidget *spin)
{
	const char *key = g_object_get_data(G_OBJECT(spin), "val");
	int value;

	value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));

	oul_prefs_set_int(key, value);
}

static void
dropdown_set(GObject *w, const char *key)
{
	const char *str_value;
	int int_value;
	OulPrefType type;

	type = GPOINTER_TO_INT(g_object_get_data(w, "type"));

	if (type == OUL_PREF_INT) {
		int_value = GPOINTER_TO_INT(g_object_get_data(w, "value"));

		oul_prefs_set_int(key, int_value);
	}
	else if (type == OUL_PREF_STRING) {
		str_value = (const char *)g_object_get_data(w, "value");

		oul_prefs_set_string(key, str_value);
	}
	else if (type == OUL_PREF_BOOLEAN) {
		oul_prefs_set_bool(key,
				GPOINTER_TO_INT(g_object_get_data(w, "value")));
	}
}


static void
delete_prefs_cb(GtkWidget *asdf, void *gdsa)
{
	/* Close any "select sound" request dialogs */
	//oul_request_close_with_handle(prefs);

	/* Unregister callbacks. */
	oul_prefs_disconnect_by_handle(prefs);

	prefs = NULL;
	beasy_sndntf_prefpage_destroy();
	notebook_page = 0;
}

static void
prefs_notebook_init(void)
{
	prefs_notebook_add_page(_("Notifications"), beasy_notify_prefpage_get(), notebook_page++);
	prefs_notebook_add_page(_("Sounds"), 		beasy_sound_prefpage_get(), notebook_page++);
	prefs_notebook_add_page(_("Emails"), 		beasy_email_prefpage_get(), notebook_page++);
	prefs_notebook_add_page(_("Themes"),		beasy_theme_prefpage_get(),	notebook_page++);
	prefs_notebook_add_page(_("Network"), 		beasy_net_prefpage_get(), notebook_page++);
}

void
beasy_prefs_show(void)
{
	GtkWidget *vbox;
	GtkWidget *notebook;
	GtkWidget *button;

	if (prefs) {
		gtk_window_present(GTK_WINDOW(prefs));
		return;
	}

	/* copy the preferences to tmp values...
	 * I liked "take affect immediately" Oh well :-( */
	/* (that should have been "effect," right?) */

	/* Back to instant-apply! I win!  BU-HAHAHA! */

	/* Create the window */
	prefs = beasy_create_dialog(_("Preferences"), BEASY_HIG_BORDER, "preferences", FALSE);
	g_signal_connect(G_OBJECT(prefs), "destroy", G_CALLBACK(delete_prefs_cb), NULL);

	vbox = beasy_dialog_get_vbox_with_properties(GTK_DIALOG(prefs), FALSE, BEASY_HIG_BORDER);

	/* The notebook */
	prefsnotebook = notebook = gtk_notebook_new ();
	gtk_box_pack_start (GTK_BOX (vbox), notebook, FALSE, FALSE, 0);
	gtk_widget_show(prefsnotebook);

	button = beasy_dialog_add_button(GTK_DIALOG(prefs), GTK_STOCK_CLOSE, NULL, NULL);
	g_signal_connect_swapped(G_OBJECT(button), "clicked",
							 G_CALLBACK(gtk_widget_destroy), prefs);

	prefs_notebook_init();

	/* Show everything. */
	gtk_widget_show(prefs);
}

GtkWidget *
beasy_prefs_checkbox(const char *text, const char *key, GtkWidget *page)
{
	GtkWidget *button;

	button = gtk_check_button_new_with_mnemonic(text);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
								 oul_prefs_get_bool(key));

	gtk_box_pack_start(GTK_BOX(page), button, FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(button), "clicked",
					 G_CALLBACK(set_bool_pref), (char *)key);

	gtk_widget_show(button);

	return button;
}

GtkWidget *
beasy_prefs_labeled_spin_button(GtkWidget *box, const gchar *title,
		const char *key, int min, int max, GtkSizeGroup *sg)
{
	GtkWidget *spin;
	GtkObject *adjust;
	int val;

	val = oul_prefs_get_int(key);

	adjust = gtk_adjustment_new(val, min, max, 1, 1, 1);
	spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjust), 1, 0);
	g_object_set_data(G_OBJECT(spin), "val", (char *)key);
	if (max < 10000)
		gtk_widget_set_size_request(spin, 50, -1);
	else
		gtk_widget_set_size_request(spin, 60, -1);
	g_signal_connect(G_OBJECT(adjust), "value-changed",
					 G_CALLBACK(update_spin_value), GTK_WIDGET(spin));
	gtk_widget_show(spin);

	return beasy_add_widget_to_vbox(GTK_BOX(box), title, sg, spin, FALSE, NULL);
}

GtkWidget *
beasy_prefs_dropdown(GtkWidget *box, const gchar *title, OulPrefType type,
			   const char *key, ...)
{
	va_list ap;
	GList *menuitems = NULL;
	GtkWidget *dropdown = NULL;
	char *name;
	int int_value;
	const char *str_value;

	g_return_val_if_fail(type == OUL_PREF_BOOLEAN || type == OUL_PREF_INT ||
			type == OUL_PREF_STRING, NULL);

	va_start(ap, key);
	while ((name = va_arg(ap, char *)) != NULL) {

		menuitems = g_list_prepend(menuitems, name);

		if (type == OUL_PREF_INT || type == OUL_PREF_BOOLEAN) {
			int_value = va_arg(ap, int);
			menuitems = g_list_prepend(menuitems, GINT_TO_POINTER(int_value));
		}
		else {
			str_value = va_arg(ap, const char *);
			menuitems = g_list_prepend(menuitems, (char *)str_value);
		}
	}
	va_end(ap);

	g_return_val_if_fail(menuitems != NULL, NULL);

	menuitems = g_list_reverse(menuitems);

	dropdown = beasy_prefs_dropdown_from_list(box, title, type, key,
			menuitems);

	g_list_free(menuitems);

	return dropdown;
}

GtkWidget *
beasy_prefs_dropdown_from_list(GtkWidget *box, const gchar *title,
		OulPrefType type, const char *key, GList *menuitems)
{
	GtkWidget  *dropdown, *opt, *menu;
	GtkWidget  *label = NULL;
	gchar      *text;
	const char *stored_str = NULL;
	int         stored_int = 0;
	int         int_value  = 0;
	const char *str_value  = NULL;
	int         o = 0;

	g_return_val_if_fail(menuitems != NULL, NULL);

	dropdown = gtk_option_menu_new();
	menu = gtk_menu_new();

	if (type == OUL_PREF_INT)
		stored_int = oul_prefs_get_int(key);
	else if (type == OUL_PREF_STRING)
		stored_str = oul_prefs_get_string(key);

	while (menuitems != NULL && (text = (char *) menuitems->data) != NULL) {
		menuitems = g_list_next(menuitems);
		g_return_val_if_fail(menuitems != NULL, NULL);

		opt = gtk_menu_item_new_with_label(text);

		g_object_set_data(G_OBJECT(opt), "type", GINT_TO_POINTER(type));

		if (type == OUL_PREF_INT) {
			int_value = GPOINTER_TO_INT(menuitems->data);
			g_object_set_data(G_OBJECT(opt), "value",
							  GINT_TO_POINTER(int_value));
		}
		else if (type == OUL_PREF_STRING) {
			str_value = (const char *)menuitems->data;

			g_object_set_data(G_OBJECT(opt), "value", (char *)str_value);
		}
		else if (type == OUL_PREF_BOOLEAN) {
			g_object_set_data(G_OBJECT(opt), "value", menuitems->data);
		}

		g_signal_connect(G_OBJECT(opt), "activate",
						 G_CALLBACK(dropdown_set), (char *)key);

		gtk_widget_show(opt);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), opt);

		if ((type == OUL_PREF_INT && stored_int == int_value) ||
			(type == OUL_PREF_STRING && stored_str != NULL &&
			 !strcmp(stored_str, str_value)) ||
			(type == OUL_PREF_BOOLEAN &&
			 (oul_prefs_get_bool(key) == GPOINTER_TO_INT(menuitems->data)))) {

			gtk_menu_set_active(GTK_MENU(menu), o);
		}

		menuitems = g_list_next(menuitems);

		o++;
	}

	gtk_option_menu_set_menu(GTK_OPTION_MENU(dropdown), menu);

	beasy_add_widget_to_vbox(GTK_BOX(box), title, NULL, dropdown, FALSE, &label);

	return label;
}

GtkWidget *
beasy_prefs_get()
{
	return prefs;
}

void
beasy_prefs_set(GtkWidget *prefs)
{
	prefs = prefs;
}

/* gtk related basic elements should be defined here */
void
beasy_prefs_init(void)
{
	oul_prefs_add_none(BEASY_PREFS_ROOT);

	beasy_notify_prefs_init();
	beasy_sound_prefs_init();
	beasy_email_prefs_init();
	beasy_net_prefs_init();
	beasy_plugin_prefs_init();
}

