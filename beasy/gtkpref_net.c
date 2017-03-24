#include "internal.h"

#include "proxy.h"
#include "network.h"
#include "prefs.h"
#include "notify.h"

#include "beasy.h"
#include "gtkutils.h"
#include "gtkprefs.h"
#include "gtkntf.h"
#include "gtkpref_net.h"

#define PROXYHOST 0
#define PROXYPORT 1
#define PROXYUSER 2
#define PROXYPASS 3

static GtkWidget *prefs_proxy_frame = NULL;

static void
proxy_changed_cb(const char *name, OulPrefType type,
				 gconstpointer value, gpointer data)
{
	GtkWidget *frame = data;
	const char *proxy = value;

	if (strcmp(proxy, "none") && strcmp(proxy, "envvar"))
		gtk_widget_show_all(frame);
	else
		gtk_widget_hide(frame);
}

static void
proxy_print_option(GtkEntry *entry, int entrynum)
{
	if (entrynum == PROXYHOST)
		oul_prefs_set_string("/oul/proxy/host", gtk_entry_get_text(entry));
	else if (entrynum == PROXYPORT)
		oul_prefs_set_int("/oul/proxy/port", atoi(gtk_entry_get_text(entry)));
	else if (entrynum == PROXYUSER)
		oul_prefs_set_string("/oul/proxy/username", gtk_entry_get_text(entry));
	else if (entrynum == PROXYPASS)
		oul_prefs_set_string("/oul/proxy/password", gtk_entry_get_text(entry));
}

static void
proxy_button_clicked_cb(GtkWidget *button, gpointer null)
{
	GError *err = NULL;

	if (g_spawn_command_line_async ("gnome-network-preferences", &err))
		return;

	oul_notify_error(NULL, NULL, _("Cannot start proxy configuration program."), err->message);
	g_error_free(err);
}


static void
browser_button_clicked_cb(GtkWidget *button, gpointer null)
{
	GError *err = NULL;

	if (g_spawn_command_line_async ("gnome-default-applications-properties", &err))
		return;

	oul_notify_error(NULL, NULL, _("Cannot start browser configuration program."), err->message);
	g_error_free(err);
}


static void
network_ip_changed(GtkEntry *entry, gpointer data)
{
	/*
	 * TODO: It would be nice if we could validate this and show a
	 *       red background in the box when the IP address is invalid
	 *       and a green background when the IP address is valid.
	 */
	oul_network_set_public_ip(gtk_entry_get_text(entry));
}


GtkWidget *
beasy_net_prefpage_get(void)
{
	GtkWidget *ret;
	GtkWidget *vbox, *hbox, *entry;
	GtkWidget *table, *label, *auto_ip_checkbox, *ports_checkbox, *spin_button;
	GtkWidget *proxy_warning = NULL, *browser_warning = NULL;
	GtkWidget *proxy_button = NULL, *browser_button = NULL;
	GtkSizeGroup *sg;
	OulProxyInfo *proxy_info = NULL;

	ret = gtk_vbox_new(FALSE, BEASY_HIG_CAT_SPACE);
	gtk_container_set_border_width (GTK_CONTAINER (ret), BEASY_HIG_BORDER);

	vbox = beasy_make_frame (ret, _("IP Address"));
	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	beasy_prefs_labeled_entry(vbox,_("ST_UN server:"),
			"/oul/network/stun_server", sg);

	hbox = gtk_hbox_new(FALSE, BEASY_HIG_BOX_SPACE);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);

	label = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(hbox), label);
	gtk_size_group_add_widget(sg, label);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
			_("<span style=\"italic\">Example: stunserver.org</span>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_container_add(GTK_CONTAINER(hbox), label);

	auto_ip_checkbox = beasy_prefs_checkbox(_("_Autodetect IP address"),
			"/oul/network/auto_ip", vbox);

	table = gtk_table_new(2, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), 0);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 10);
	gtk_container_add(GTK_CONTAINER(vbox), table);

	label = gtk_label_new_with_mnemonic(_("Public _IP:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
	gtk_size_group_add_widget(sg, label);

	entry = gtk_entry_new();
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);
	g_signal_connect(G_OBJECT(entry), "changed",
					 G_CALLBACK(network_ip_changed), NULL);

	/*
	 * TODO: This could be better by showing the autodeteced
	 * IP separately from the user-specified IP.
	 */
	if (oul_network_get_my_ip(-1) != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry),
		                   oul_network_get_my_ip(-1));

	beasy_set_accessible_label (entry, label);


	if (oul_prefs_get_bool("/oul/network/auto_ip")) {
		gtk_widget_set_sensitive(GTK_WIDGET(table), FALSE);
	}

	g_signal_connect(G_OBJECT(auto_ip_checkbox), "clicked",
					 G_CALLBACK(beasy_toggle_sensitive), table);

	g_object_unref(sg);

	vbox = beasy_make_frame (ret, _("Ports"));
	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	beasy_prefs_checkbox(_("_Enable automatic router port forwarding"),
			"/oul/network/map_ports", vbox);

	ports_checkbox = beasy_prefs_checkbox(_("_Manually specify range of ports to listen on"),
			"/oul/network/ports_range_use", vbox);

	spin_button = beasy_prefs_labeled_spin_button(vbox, _("_Start port:"),
			"/oul/network/ports_range_start", 0, 65535, sg);
	if (!oul_prefs_get_bool("/oul/network/ports_range_use"))
		gtk_widget_set_sensitive(GTK_WIDGET(spin_button), FALSE);
	g_signal_connect(G_OBJECT(ports_checkbox), "clicked",
					 G_CALLBACK(beasy_toggle_sensitive), spin_button);

	spin_button = beasy_prefs_labeled_spin_button(vbox, _("_End port:"),
			"/oul/network/ports_range_end", 0, 65535, sg);
	if (!oul_prefs_get_bool("/oul/network/ports_range_use"))
		gtk_widget_set_sensitive(GTK_WIDGET(spin_button), FALSE);
	g_signal_connect(G_OBJECT(ports_checkbox), "clicked",
					 G_CALLBACK(beasy_toggle_sensitive), spin_button);

	if (oul_running_gnome()) {
		vbox = beasy_make_frame(ret, _("Proxy Server &amp; Browser"));
		prefs_proxy_frame = gtk_vbox_new(FALSE, 0);

		proxy_warning = hbox = gtk_hbox_new(FALSE, BEASY_HIG_BOX_SPACE);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);

		label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label),
		                     _("<b>Proxy configuration program was not found.</b>"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

		browser_warning = hbox = gtk_hbox_new(FALSE, BEASY_HIG_BOX_SPACE);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);

		label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label),
		                     _("<b>Browser configuration program was not found.</b>"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

		hbox = gtk_hbox_new(FALSE, BEASY_HIG_BOX_SPACE);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);
		label = gtk_label_new(_("Proxy & Browser preferences are configured\n"
		                        "in GNOME Preferences"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
		gtk_widget_show(label);

		hbox = gtk_hbox_new(FALSE, BEASY_HIG_BOX_SPACE);
		gtk_container_add(GTK_CONTAINER(vbox), hbox);
		proxy_button = gtk_button_new_with_mnemonic(_("Configure _Proxy"));
		g_signal_connect(G_OBJECT(proxy_button), "clicked",
		                 G_CALLBACK(proxy_button_clicked_cb), NULL);
		gtk_box_pack_start(GTK_BOX(hbox), proxy_button, FALSE, FALSE, 0);
		gtk_widget_show(proxy_button);
		browser_button = gtk_button_new_with_mnemonic(_("Configure _Browser"));
		g_signal_connect(G_OBJECT(browser_button), "clicked",
		                 G_CALLBACK(browser_button_clicked_cb), NULL);
		gtk_box_pack_start(GTK_BOX(hbox), browser_button, FALSE, FALSE, 0);
		gtk_widget_show(browser_button);
	} else {
		vbox = beasy_make_frame(ret, _("Proxy Server"));
		prefs_proxy_frame = gtk_vbox_new(FALSE, 0);

		beasy_prefs_dropdown(vbox, _("Proxy _type:"), OUL_PREF_STRING,
					"/oul/proxy/type",
					_("No proxy"), "none",
					"SOCKS 4", "socks4",
					"SOCKS 5", "socks5",
					"HTTP", "http",
					_("Use Environmental Settings"), "envvar",
					NULL);
		gtk_box_pack_start(GTK_BOX(vbox), prefs_proxy_frame, 0, 0, 0);
		proxy_info = oul_global_proxy_get_info();

		oul_prefs_connect_callback(beasy_prefs_get(), "/oul/proxy/type", 
									proxy_changed_cb, prefs_proxy_frame);

		table = gtk_table_new(4, 2, FALSE);
		gtk_container_set_border_width(GTK_CONTAINER(table), 0);
		gtk_table_set_col_spacings(GTK_TABLE(table), 5);
		gtk_table_set_row_spacings(GTK_TABLE(table), 10);
		gtk_container_add(GTK_CONTAINER(prefs_proxy_frame), table);


		label = gtk_label_new_with_mnemonic(_("_Host:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

		entry = gtk_entry_new();
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);
		gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);
		g_signal_connect(G_OBJECT(entry), "changed",
				 G_CALLBACK(proxy_print_option), (void *)PROXYHOST);

		if (proxy_info != NULL && oul_proxy_info_get_host(proxy_info))
			gtk_entry_set_text(GTK_ENTRY(entry),
					   oul_proxy_info_get_host(proxy_info));

		hbox = gtk_hbox_new(TRUE, 5);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
		beasy_set_accessible_label (entry, label);

		label = gtk_label_new_with_mnemonic(_("_Port:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

		entry = gtk_entry_new();
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);
		gtk_table_attach(GTK_TABLE(table), entry, 3, 4, 0, 1, GTK_FILL, 0, 0, 0);
		g_signal_connect(G_OBJECT(entry), "changed",
				 G_CALLBACK(proxy_print_option), (void *)PROXYPORT);

		if (proxy_info != NULL && oul_proxy_info_get_port(proxy_info) != 0) {
			char buf[128];
			g_snprintf(buf, sizeof(buf), "%d",
				   oul_proxy_info_get_port(proxy_info));

			gtk_entry_set_text(GTK_ENTRY(entry), buf);
		}
		beasy_set_accessible_label (entry, label);

		label = gtk_label_new_with_mnemonic(_("_User:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);

		entry = gtk_entry_new();
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);
		gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
		g_signal_connect(G_OBJECT(entry), "changed",
				 G_CALLBACK(proxy_print_option), (void *)PROXYUSER);

		if (proxy_info != NULL && oul_proxy_info_get_username(proxy_info) != NULL)
			gtk_entry_set_text(GTK_ENTRY(entry),
						   oul_proxy_info_get_username(proxy_info));

		hbox = gtk_hbox_new(TRUE, 5);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
		beasy_set_accessible_label (entry, label);

		label = gtk_label_new_with_mnemonic(_("Pa_ssword:"));
		gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);

		entry = gtk_entry_new();
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);
		gtk_table_attach(GTK_TABLE(table), entry, 3, 4, 1, 2, GTK_FILL , 0, 0, 0);
		gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
		if (gtk_entry_get_invisible_char(GTK_ENTRY(entry)) == '*')
			gtk_entry_set_invisible_char(GTK_ENTRY(entry), BEASY_INVISIBLE_CHAR);
		g_signal_connect(G_OBJECT(entry), "changed",
				 G_CALLBACK(proxy_print_option), (void *)PROXYPASS);

		if (proxy_info != NULL && oul_proxy_info_get_password(proxy_info) != NULL)
			gtk_entry_set_text(GTK_ENTRY(entry),
					   oul_proxy_info_get_password(proxy_info));
		beasy_set_accessible_label (entry, label);
	}

	gtk_widget_show_all(ret);
	g_object_unref(sg);
	/* Only hide table if not running gnome otherwise we hide the IP address table! */
	if (!oul_running_gnome() && (proxy_info == NULL ||
	    oul_proxy_info_get_type(proxy_info) == OUL_PROXY_NONE ||
	    oul_proxy_info_get_type(proxy_info) == OUL_PROXY_USE_ENVVAR)) {
		gtk_widget_hide(table);
	} else if (oul_running_gnome()) {
		gchar *path;
		path = g_find_program_in_path("gnome-network-preferences");
		if (path != NULL) {
			gtk_widget_set_sensitive(proxy_button, TRUE);
			gtk_widget_hide(proxy_warning);
			g_free(path);
		} else {
			gtk_widget_set_sensitive(proxy_button, FALSE);
			gtk_widget_show(proxy_warning);
		}
		path = g_find_program_in_path("gnome-default-applications-properties");
		if (path != NULL) {
			gtk_widget_set_sensitive(browser_button, TRUE);
			gtk_widget_hide(browser_warning);
			g_free(path);
		} else {
			gtk_widget_set_sensitive(browser_button, FALSE);
			gtk_widget_show(browser_warning);
		}
	}

	return ret;
}

void
beasy_net_prefs_init()
{
	/* Browsers */
	oul_prefs_add_none(BEASY_PREFS_NET_BROWSERS_ROOT);
	oul_prefs_add_int(BEASY_PREFS_NET_BROWSERS_PLACE, BEASY_BROWSER_DEFAULT);
	oul_prefs_add_path(BEASY_PREFS_NET_BROWSERS_COMMAND, "");
	oul_prefs_add_string(BEASY_PREFS_NET_BROWSERS_BROWSER, "mozilla");
}
