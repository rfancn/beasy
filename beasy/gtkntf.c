#include "internal.h"

#include <gdk/gdkkeysyms.h>

#include "notify.h"
#include "msg.h"

#include "beasy.h"
#include "beasystock.h"
#include "gtkhtml.h"
#include "gtkutils.h"

#include "gtkntf_action.h"
#include "gtkntf_display.h"
#include "gtkntf_event.h"
#include "gtkntf_gtk_utils.h"
#include "gtkntf_item_text.h"
#include "gtkntf_theme.h"
#include "gtkpref_net.h"


void *
gtkntf_get_handle()
{
	static gint handle;

	return &handle;
}

void
beasy_notify_init()
{

	gtkntf_actions_init();
	gtkntf_events_init();
	gtkntf_item_text_init();
	gtkntf_display_init();
	gtkntf_gtk_utils_init();
	gtkntf_themes_probe();
	gtkntf_themes_load_saved();
}

void
beasy_notify_uninit() {
	time_t t;

	/* seed the random number generator here so that two notifications that
	 * occur within a second of each other use different notifications.
	 */
	t = time(NULL);
	srand(t);

	gtkntf_themes_unprobe();
	gtkntf_themes_save_loaded();
	gtkntf_themes_unload();
	gtkntf_gtk_utils_uninit();
	gtkntf_display_uninit();
	gtkntf_item_text_uninit();
	gtkntf_events_uninit();
	gtkntf_actions_uninit();

}

static GtkHtmlOptions
notify_html_options(void)
{
	GtkHtmlOptions options = 0;

	options |= GTK_HTML_NO_COMMENTS;
	options |= GTK_HTML_NO_TITLE;
	options |= GTK_HTML_NO_NEWLINE;
	options |= GTK_HTML_NO_SCROLL;
	
	return options;
}


static gboolean
formatted_input_cb(GtkWidget *win, GdkEventKey *event, gpointer data)
{
	if (event->keyval == GDK_Escape)
	{
		oul_notify_close(OUL_NOTIFY_FORMATTED, win);

		return TRUE;
	}

	return FALSE;
}


static void
beasy_close_notify(OulNotifyType type, void *ui_handle)
{
	if (ui_handle != NULL)
		gtk_widget_destroy(GTK_WIDGET(ui_handle));
}

static gboolean
formatted_close_cb(GtkWidget *win, GdkEvent *event, void *user_data)
{
	oul_notify_close(OUL_NOTIFY_FORMATTED, win);
	return FALSE;
}

static gboolean
table_close_cb(GtkWidget *win, GdkEvent *event, void *user_data)
{
	oul_notify_close(OUL_NOTIFY_TABLE, win);
	return FALSE;
}

static gboolean
table_input_cb(GtkWidget *win, GdkEventKey *event, gpointer data)
{
	if (event->keyval == GDK_Escape)
	{
		oul_notify_close(OUL_NOTIFY_FORMATTED, win);

		return TRUE;
	}

	return FALSE;
}

static GtkWidget *
table_list_view_new(const OulMsg *msg)
{
	GType *types;
	int ncolumn, i;
	GtkListStore *liststore;
	GtkCellRenderer *rend;
	GtkTreeViewColumn *col;
	GtkTreeIter iter;
	GtkWidget *listview;
	GList *plist;

	ncolumn = g_list_length(msg->content.table->headers);

	/* new liststore */
	types = g_new0(GType, ncolumn);
	for(i=0; i<ncolumn; i++){
		types[i] = G_TYPE_STRING;
	}

	liststore = gtk_list_store_newv(ncolumn, types);
	g_free(types);

	listview  = gtk_tree_view_new_with_model(GTK_TREE_MODEL(liststore));
	g_object_unref(G_OBJECT(liststore));

	/* new table header */
	for(i=0, plist = msg->content.table->headers; plist; plist=g_list_next(plist), i++){
		OulMsgTbHeader *tbheader = (OulMsgTbHeader *)plist->data;

		rend = gtk_cell_renderer_text_new();
		col = gtk_tree_view_column_new_with_attributes (_(tbheader->name), rend,
							"text", i, NULL);
		
		gtk_tree_view_append_column (GTK_TREE_VIEW(listview), col);
	}

	/* new table rows */
	GList *cells = NULL;
	for(plist = msg->content.table->rows; plist; plist = g_list_next(plist)){
		OulMsgTbRow *row = (OulMsgTbRow *)plist->data;

		gtk_list_store_append (liststore, &iter);

		cells = row->cells;
		for(i=0; cells; cells=cells->next, i++){
			OulMsgTbCell *cell = (OulMsgTbCell *)cells->data;

			gtk_list_store_set(GTK_LIST_STORE(liststore), &iter, cell->col, cell->content, -1);
		}

		
	}
				
	return listview;

}

static void
message_response_cb(GtkDialog *dialog, gint id, GtkWidget *widget)
{
	oul_notify_close(OUL_NOTIFY_MESSAGE, widget);
}

static void *
beasy_notify_message(OulNotifyMsgType type, const char *title,
						const char *primary, const char *secondary)
{
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *img = NULL;
	char label_text[2048];
	const char *icon_name = NULL;
	char *primary_esc, *secondary_esc;

	switch (type)
	{
		case OUL_NOTIFY_MSG_ERROR:
			icon_name = BEASY_STOCK_DIALOG_ERROR;
			break;

		case OUL_NOTIFY_MSG_WARNING:
			icon_name = BEASY_STOCK_DIALOG_WARNING;
			break;

		case OUL_NOTIFY_MSG_INFO:
			icon_name = BEASY_STOCK_DIALOG_INFO;
			break;

		default:
			icon_name = NULL;
			break;
	}

	if (icon_name != NULL)
	{
		img = gtk_image_new_from_stock(icon_name, gtk_icon_size_from_name(BEASY_ICON_SIZE_TANGO_HUGE));
		gtk_misc_set_alignment(GTK_MISC(img), 0, 0);
	}

	dialog = gtk_dialog_new_with_buttons(title ? title : BEASY_ALERT_TITLE,
										 NULL, 0, GTK_STOCK_CLOSE,
										 GTK_RESPONSE_CLOSE, NULL);

	gtk_window_set_role(GTK_WINDOW(dialog), "notify_dialog");

	g_signal_connect(G_OBJECT(dialog), "response",
					 G_CALLBACK(message_response_cb), dialog);

	gtk_container_set_border_width(GTK_CONTAINER(dialog), BEASY_HIG_BORDER);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
	gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog)->vbox), BEASY_HIG_BORDER);
	gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), BEASY_HIG_BOX_SPACE);

	hbox = gtk_hbox_new(FALSE, BEASY_HIG_BORDER);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);

	if (img != NULL)
		gtk_box_pack_start(GTK_BOX(hbox), img, FALSE, FALSE, 0);

	primary_esc = g_markup_escape_text(primary, -1);
	secondary_esc = (secondary != NULL) ? g_markup_escape_text(secondary, -1) : NULL;
	g_snprintf(label_text, sizeof(label_text),
			   "<span weight=\"bold\" size=\"larger\">%s</span>%s%s",
			   primary_esc, (secondary ? "\n\n" : ""),
			   (secondary ? secondary_esc : ""));
	g_free(primary_esc);
	g_free(secondary_esc);

	label = gtk_label_new(NULL);

	gtk_label_set_markup(GTK_LABEL(label), label_text);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	beasy_auto_parent_window(dialog);

	gtk_widget_show_all(dialog);

	return dialog;
}

static void *
beasy_notify_formatted(const char *title, const char *primary,
						  const char *secondary, const char *text, int width, int height)
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *button;
	GtkWidget *html;
	GtkWidget *frame;
	char label_text[2048];
	char *linked_text, *primary_esc, *secondary_esc;

	window = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(window), title);
	gtk_container_set_border_width(GTK_CONTAINER(window), BEASY_HIG_BORDER);
	gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(formatted_close_cb), NULL);

	/* Setup the main vbox */
	vbox = GTK_DIALOG(window)->vbox;

	/* Setup the descriptive label */
	primary_esc = g_markup_escape_text(primary, -1);
	secondary_esc = (secondary != NULL) ? g_markup_escape_text(secondary, -1) : NULL;
	g_snprintf(label_text, sizeof(label_text),
			   "<span weight=\"bold\" size=\"larger\">%s</span>%s%s",
			   primary_esc,
			   (secondary ? "\n" : ""),
			   (secondary ? secondary_esc : ""));
	g_free(primary_esc);
	g_free(secondary_esc);

	label = gtk_label_new(NULL);

	gtk_label_set_markup(GTK_LABEL(label), label_text);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	/* Add the html */
	frame = beasy_create_html(FALSE, &html, NULL);
	gtk_widget_set_name(html, "beasy_notify_html");
	gtk_html_set_format_functions(GTK_HTML(html),
			gtk_html_get_format_functions(GTK_HTML(html)) | GTK_HTML_IMAGE);

	if(width  < 240 || width > 960) width = 300;
	if(height < 120 || height > 600) height = 250;
	
	gtk_widget_set_size_request(html, width, height);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	gtk_widget_show(frame);

	/* Add the Close button. */
	button = gtk_dialog_add_button(GTK_DIALOG(window), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_widget_grab_focus(button);

	g_signal_connect_swapped(G_OBJECT(button), "clicked",
							 G_CALLBACK(gtk_widget_destroy), window);
	g_signal_connect(G_OBJECT(window), "key_press_event",
					 G_CALLBACK(formatted_input_cb), NULL);

	/* Make sure URLs are clickable */
	linked_text = oul_markup_linkify(text);
	gtk_html_append_text(GTK_HTML(html), linked_text, notify_html_options());
	g_free(linked_text);

	g_object_set_data(G_OBJECT(window), "info-widget", html);

	/* Show the window */
	beasy_auto_parent_window(window);

	gtk_widget_show(window);

	return window;
}

static void *
beasy_notify_table(const char *title,const OulMsg *msg)
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *button;
	GtkWidget *sw;
	GtkWidget *listview;

	g_return_if_fail(msg != NULL);
	
	/* new window */
	window = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(window), title);
	gtk_container_set_border_width(GTK_CONTAINER(window), BEASY_HIG_BORDER);
	gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(table_close_cb), NULL);

	/* setup the main vbox */
	vbox = GTK_DIALOG(window)->vbox;

	/* setup the scroll window */
	sw = gtk_scrolled_window_new(NULL,NULL);

	gtk_widget_set_size_request(sw, -1, 300);
	
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

	/* setup the table based on beasymsg */
	listview = table_list_view_new(msg);

	gtk_container_add(GTK_CONTAINER(sw), listview);

	/* Add the Close button. */
	button = gtk_dialog_add_button(GTK_DIALOG(window), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_widget_grab_focus(button);

	g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(gtk_widget_destroy), window);
	g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(table_input_cb), NULL);

	/* show the window */
	beasy_auto_parent_window(window);

	gtk_widget_show_all(window);

	return window;
}

static gint
uri_command(const char *command, gboolean sync)
{
	gchar *tmp;
	GError *error = NULL;
	gint ret = 0;

	oul_debug_misc("gtknotify", "Executing %s\n", command);

	if (!oul_program_is_valid(command))
	{
		tmp = g_strdup_printf(_("The browser command \"%s\" is invalid."),
							  command ? command : "(none)");
		oul_notify_error(NULL, NULL, _("Unable to open URL"), tmp);
		g_free(tmp);

	}
	else if (sync)
	{
		gint status;

		if (!g_spawn_command_line_sync(command, NULL, NULL, &status, &error))
		{
			tmp = g_strdup_printf(_("Error launching \"%s\": %s"),
										command, error->message);
			oul_notify_error(NULL, NULL, _("Unable to open URL"), tmp);
			g_free(tmp);
			g_error_free(error);
		}
		else
			ret = status;
	}
	else
	{
		if (!g_spawn_command_line_async(command, &error))
		{
			tmp = g_strdup_printf(_("Error launching \"%s\": %s"),
										command, error->message);
			oul_notify_error(NULL, NULL, _("Unable to open URL"), tmp);
			g_free(tmp);
			g_error_free(error);
		}
	}

	return ret;
}

static void *
beasy_notify_uri(const char *uri)
{
	char *escaped = g_shell_quote(uri);
	char *command = NULL;
	char *remote_command = NULL;
	const char *web_browser;
	int place;

	web_browser = oul_prefs_get_string(BEASY_PREFS_NET_BROWSERS_BROWSER);
	place = oul_prefs_get_int(BEASY_PREFS_NET_BROWSERS_PLACE);

	/* if they are running gnome, use the gnome web browser */
	if (oul_running_gnome() == TRUE)
	{
		char *tmp = g_find_program_in_path("xdg-open");
		if (tmp == NULL)
			command = g_strdup_printf("gnome-open %s", escaped);
		else
			command = g_strdup_printf("xdg-open %s", escaped);
		g_free(tmp);
	}
	else if (!strcmp(web_browser, "epiphany") ||
		!strcmp(web_browser, "galeon"))
	{
		if (place == BEASY_BROWSER_NEW_WINDOW)
			command = g_strdup_printf("%s -w %s", web_browser, escaped);
		else if (place == BEASY_BROWSER_NEW_TAB)
			command = g_strdup_printf("%s -n %s", web_browser, escaped);
		else
			command = g_strdup_printf("%s %s", web_browser, escaped);
	}
	else if (!strcmp(web_browser, "xdg-open"))
	{
		command = g_strdup_printf("xdg-open %s", escaped);
	}
	else if (!strcmp(web_browser, "gnome-open"))
	{
		command = g_strdup_printf("gnome-open %s", escaped);
	}
	else if (!strcmp(web_browser, "kfmclient"))
	{
		command = g_strdup_printf("kfmclient openURL %s", escaped);
		/*
		 * Does Konqueror have options to open in new tab
		 * and/or current window?
		 */
	}
	else if (!strcmp(web_browser, "mozilla") ||
			 !strcmp(web_browser, "mozilla-firebird") ||
			 !strcmp(web_browser, "firefox") ||
			 !strcmp(web_browser, "seamonkey"))
	{
		char *args = "";

		command = g_strdup_printf("%s %s", web_browser, escaped);

		/*
		 * Firefox 0.9 and higher require a "-a firefox" option when
		 * using -remote commands.  This breaks older versions of
		 * mozilla.  So we include this other handly little string
		 * when calling firefox.  If the API for remote calls changes
		 * any more in firefox then firefox should probably be split
		 * apart from mozilla-firebird and mozilla... but this is good
		 * for now.
		 */
		if (!strcmp(web_browser, "firefox"))
			args = "-a firefox";

		if (place == BEASY_BROWSER_NEW_WINDOW)
			remote_command = g_strdup_printf("%s %s -remote "
											 "openURL(%s,new-window)",
											 web_browser, args, escaped);
		else if (place == BEASY_BROWSER_NEW_TAB)
			remote_command = g_strdup_printf("%s %s -remote "
											 "openURL(%s,new-tab)",
											 web_browser, args, escaped);
		else if (place == BEASY_BROWSER_CURRENT)
			remote_command = g_strdup_printf("%s %s -remote "
											 "openURL(%s)",
											 web_browser, args, escaped);
	}
	else if (!strcmp(web_browser, "netscape"))
	{
		command = g_strdup_printf("netscape %s", escaped);

		if (place == BEASY_BROWSER_NEW_WINDOW)
		{
			remote_command = g_strdup_printf("netscape -remote "
											 "openURL(%s,new-window)",
											 escaped);
		}
		else if (place == BEASY_BROWSER_CURRENT)
		{
			remote_command = g_strdup_printf("netscape -remote "
											 "openURL(%s)", escaped);
		}
	}
	else if (!strcmp(web_browser, "opera"))
	{
		if (place == BEASY_BROWSER_NEW_WINDOW)
			command = g_strdup_printf("opera -newwindow %s", escaped);
		else if (place == BEASY_BROWSER_NEW_TAB)
			command = g_strdup_printf("opera -newpage %s", escaped);
		else if (place == BEASY_BROWSER_CURRENT)
		{
			remote_command = g_strdup_printf("opera -remote "
											 "openURL(%s)", escaped);
			command = g_strdup_printf("opera %s", escaped);
		}
		else
			command = g_strdup_printf("opera %s", escaped);

	}
	else if (!strcmp(web_browser, "custom"))
	{
		const char *web_command;

		web_command = oul_prefs_get_path(BEASY_PREFS_NET_BROWSERS_COMMAND);

		if (web_command == NULL || *web_command == '\0')
		{
			oul_notify_error(NULL, NULL, _("Unable to open URL"),
							  _("The 'Manual' browser command has been "
								"chosen, but no command has been set."));
			return NULL;
		}

		if (strstr(web_command, "%s"))
			command = oul_strreplace(web_command, "%s", escaped);
		else
		{
			/*
			 * There is no "%s" in the browser command.  Assume the user
			 * wanted the URL tacked on to the end of the command.
			 */
			command = g_strdup_printf("%s %s", web_command, escaped);
		}
	}

	g_free(escaped);

	if (remote_command != NULL)
	{
		/* try the remote command first */
		if (uri_command(remote_command, TRUE) != 0)
			uri_command(command, FALSE);

		g_free(remote_command);

	}
	else
		uri_command(command, FALSE);

	g_free(command);

	return NULL;
}



static OulNotifyUiOps ops =
{
	beasy_notify_init,
	beasy_notify_uninit,
	beasy_notify_message,
	beasy_notify_formatted,
	beasy_notify_table,
	beasy_notify_uri,
	beasy_close_notify,
	NULL,
	NULL
};

OulNotifyUiOps *
beasy_notify_get_ui_ops(void)
{
	return &ops;
}

