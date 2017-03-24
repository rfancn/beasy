#include "internal.h"

#include "beasy.h"
#include "gtkpref_email.h"

static void
email_enabled_toggle_cb(GtkWidget *widget, gpointer data)
{
	gboolean on = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	oul_prefs_set_bool(BEASY_PREFS_EMAIL_ENABLED, on);
	
	GtkWidget *entry = g_object_get_data(G_OBJECT(widget), "email_address");
    gtk_widget_set_sensitive(entry, on);

    oul_prefs_set_string(BEASY_PREFS_EMAIL_ADDRESS, 
						gtk_entry_get_text(GTK_ENTRY(entry)));
}

static void
email_address_changed_cb(GtkWidget *widget, gpointer data)
{
	oul_prefs_set_string(BEASY_PREFS_EMAIL_ADDRESS, 
						gtk_entry_get_text(GTK_ENTRY(widget)));
}

GtkWidget *
beasy_email_prefpage_get(void)
{
	GtkWidget *ret = NULL, *frame = NULL;
	GtkWidget *vbox = NULL, *hbox = NULL;
	GtkWidget *toggle = NULL, *entry = NULL;
	
	ret = gtk_vbox_new(FALSE, BEASY_HIG_CAT_SPACE);
	gtk_container_set_border_width(GTK_CONTAINER (ret), BEASY_HIG_BORDER);

	frame = beasy_make_frame(ret, _("Email Setting"));
	vbox = gtk_vbox_new(FALSE, BEASY_HIG_BOX_SPACE);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	
	hbox = gtk_hbox_new(FALSE, BEASY_HIG_CAT_SPACE);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	toggle = gtk_check_button_new_with_mnemonic(_("Enable Email:"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), 
								oul_prefs_get_bool(BEASY_PREFS_EMAIL_ENABLED));
	gtk_box_pack_start(GTK_BOX(hbox), toggle, FALSE, FALSE, 0);

	
    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
    gtk_entry_set_max_length(GTK_ENTRY(entry), 45);
    gtk_widget_set_sensitive(GTK_WIDGET(entry),
                             oul_prefs_get_bool(BEASY_PREFS_EMAIL_ENABLED));
    gtk_entry_set_text(GTK_ENTRY(entry),
                       oul_prefs_get_string(BEASY_PREFS_EMAIL_ADDRESS));
    g_object_set_data(G_OBJECT(toggle), "email_address", entry);
    g_signal_connect(G_OBJECT(toggle), "toggled",
                     G_CALLBACK(email_enabled_toggle_cb), NULL);
    g_signal_connect(G_OBJECT(entry), "focus-out-event",
                     G_CALLBACK(email_address_changed_cb), NULL);

	gtk_widget_show_all(ret);

	return ret;
}


void
beasy_email_prefs_init()
{
	oul_prefs_add_none(BEASY_PREFS_EMAIL_ROOT);

	oul_prefs_add_bool(BEASY_PREFS_EMAIL_ENABLED, FALSE);
	oul_prefs_add_string(BEASY_PREFS_EMAIL_ADDRESS, "");

}

