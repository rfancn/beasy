/*
 * @file gtkdialogs.c GTK+ Dialogs
 * @ingroup beasy
 */

#include "internal.h"

#include "beasy.h"
#include "beasystock.h"

#include "gtkhtml.h"
#include "gtkutils.h"
#include "gtkdialogs.h"

#include "debug.h"
#include "notify.h"
#include "util.h"
#include "plugin.h"

static GList *dialogwindows = NULL;

static GtkWidget *about = NULL;

struct developer {
	char *name;
	char *role;
	char *email;
};

struct translator {
	char *language;
	char *abbr;
	char *name;
	char *email;
};

struct artist {
	char *name;
	char *email;
};

/* Order: Lead Developer, then Alphabetical by Last Name */
static const struct developer developers[] = {
	{"Ryan Fan",					N_("lead developer"), "rfan.cn@gmail.com"},
	{NULL, NULL, NULL}
};

/* Order: Alphabetical by Last Name */
static const struct developer patch_writers[] = {
	{NULL, NULL, NULL}
};

static const struct artist artists[] = {
	{"Ryan Fan",	"rfan.cn@gmail.com"},
	{NULL, NULL}
};


/* Order: Code, then Alphabetical by Last Name */
static const struct translator current_translators[] = {
	{N_("Simplified Chinese"),  "zh_CN", "Ryan Fan", "rfan.cn@gmail.com"},
	{NULL, NULL, NULL, NULL}
};

void
beasy_dialogs_destroy_all()
{
	while (dialogwindows) {
		gtk_widget_destroy(dialogwindows->data);
		dialogwindows = g_list_remove(dialogwindows, dialogwindows->data);
	}
}

static void destroy_about(void)
{
	if (about != NULL)
		gtk_widget_destroy(about);
	about = NULL;
}

void beasy_dialogs_about()
{
	GtkWidget *vbox;
	GtkWidget *logo;
	GtkWidget *frame;
	GtkWidget *text;
	GtkWidget *button;
	GtkTextIter iter;
	GString *str;
	int i;
	AtkObject *obj;
	char* filename, *tmp;
	GdkPixbuf *pixbuf;

	if (about != NULL) {
		gtk_window_present(GTK_WINDOW(about));
		return;
	}

	tmp = g_strdup_printf(_("About %s"), BEASY_NAME);
	about = beasy_create_dialog(tmp, BEASY_HIG_BORDER, "about", TRUE);
	g_free(tmp);
	gtk_window_set_default_size(GTK_WINDOW(about), 340, 450);

	vbox = beasy_dialog_get_vbox_with_properties(GTK_DIALOG(about), FALSE, BEASY_HIG_BORDER);

	/* Generate a logo with a version number */
	logo = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_realize(logo);
	filename = g_build_filename(PKGDATADIR, "pixmaps", "logo.png", NULL);
	pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
	g_free(filename);
#if 0  /* Don't versionize the logo when the logo has the version in it */
	beasy_logo_versionize(&pixbuf, logo);
#endif
	gtk_widget_destroy(logo);
	logo = gtk_image_new_from_pixbuf(pixbuf);
	gdk_pixbuf_unref(pixbuf);
	/* Insert the logo */
	obj = gtk_widget_get_accessible(logo);
	tmp = g_strconcat(BEASY_NAME, "0.0.1", NULL);
	atk_object_set_description(obj, tmp);
	g_free(tmp);
	gtk_box_pack_start(GTK_BOX(vbox), logo, FALSE, FALSE, 0);

	frame = beasy_create_html(FALSE, &text, NULL);
	gtk_html_set_format_functions(GTK_HTML(text), GTK_HTML_ALL ^ GTK_HTML_SMILEY);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

	str = g_string_sized_new(4096);

	g_string_append_printf(str,
		"<CENTER><FONT SIZE=\"4\"><B>%s %s</B></FONT></CENTER><BR><BR>", BEASY_NAME, "0.0.1");

	g_string_append_printf(str,
		_("%s is a plugin based notify client based on "
		  "liboul which is capable of notify various events"
		  "It is written using GTK+.<BR><BR>"
		  "You may modify and redistribute the program under "
		  "the terms of the GPL (version 2 or later).  A copy of the GPL is "
		  "contained in the 'COPYING' file distributed with %s.  "
		  "%s is copyrighted by its contributors.  See the 'COPYRIGHT' "
		  "file for the complete list of contributors.  We provide no "
		  "warranty for this program.<BR><BR>"), BEASY_NAME, BEASY_NAME, BEASY_NAME);

	g_string_append(str, "<FONT SIZE=\"4\">URL:</FONT> <A HREF=\""
					OUL_WEBSITE "\">" OUL_WEBSITE "</A><BR/><BR/>");
	g_string_append(str, "<FONT SIZE=\"4\">FAQ:</FONT> <A HREF=\""
			"http://rfan.512j.com\">"
			"http://rfan.512j.com</A><BR/><BR/>");
	g_string_append_printf(str, _("<FONT SIZE=\"4\">IRC:</FONT> "
						   "#beasy on hangchecktimer<BR><BR>"));

	/* Current Developers */
	g_string_append_printf(str, "<FONT SIZE=\"4\">%s:</FONT><BR/>",
						   _("Current Developers"));
	for (i = 0; developers[i].name != NULL; i++) {
		if (developers[i].email != NULL) {
			g_string_append_printf(str, "  %s (%s) &lt;<a href=\"mailto:%s\">%s</a>&gt;<br/>",
					developers[i].name, _(developers[i].role),
					developers[i].email, developers[i].email);
		} else {
			g_string_append_printf(str, "  %s (%s)<br/>",
					developers[i].name, _(developers[i].role));
		}
	}
	g_string_append(str, "<BR/>");

	/* Crazy Patch Writers */
	g_string_append_printf(str, "<FONT SIZE=\"4\">%s:</FONT><BR/>",
						   _("Crazy Patch Writers"));
	for (i = 0; patch_writers[i].name != NULL; i++) {
		if (patch_writers[i].email != NULL) {
			g_string_append_printf(str, "  %s &lt;<a href=\"mailto:%s\">%s</a>&gt;<br/>",
					patch_writers[i].name,
					patch_writers[i].email, patch_writers[i].email);
		} else {
			g_string_append_printf(str, "  %s<br/>",
					patch_writers[i].name);
		}
	}
	g_string_append(str, "<BR/>");

	/* Artists */
        g_string_append_printf(str, "<FONT SIZE=\"4\">%s:</FONT><BR/>",
                                                   _("Artists"));
        for (i = 0; artists[i].name != NULL; i++) {
        	if (artists[i].email != NULL) {
			g_string_append_printf(str, "  %s &lt;<a href=\"mailto:%s\">%s</a>&gt;<br/>",
			                           artists[i].name,
			                           artists[i].email, artists[i].email);
	        } else {
	                g_string_append_printf(str, "  %s<br/>",
	                                      artists[i].name);
	        }
	}
	g_string_append(str, "<BR/>");
			
	/* Current Translators */
	g_string_append_printf(str, "<FONT SIZE=\"4\">%s:</FONT><BR/>",
						   _("Current Translators"));
	for (i = 0; current_translators[i].language != NULL; i++) {
		if (current_translators[i].email != NULL) {
			g_string_append_printf(str, "  <b>%s (%s)</b> - %s &lt;<a href=\"mailto:%s\">%s</a>&gt;<br/>",
							_(current_translators[i].language),
							current_translators[i].abbr,
							_(current_translators[i].name),
							current_translators[i].email,
							current_translators[i].email);
		} else {
			g_string_append_printf(str, "  <b>%s (%s)</b> - %s<br/>",
							_(current_translators[i].language),
							current_translators[i].abbr,
							_(current_translators[i].name));
		}
	}
	g_string_append(str, "<BR/>");

	g_string_append_printf(str, "<FONT SIZE=\"4\">%s</FONT><br/>", _("Debugging Information"));

	/* The following primarly intented for user/developer interaction and thus
	   ought not be translated */

#ifdef CONFIG_ARGS
	g_string_append(str, "  <b>Arguments to <i>./configure</i>:</b>  " CONFIG_ARGS "<br/>");
#endif

#ifdef DEBUG
	g_string_append(str, "  <b>Print debugging messages:</b> Yes<br/>");
#else
	g_string_append(str, "  <b>Print debugging messages:</b> No<br/>");
#endif


#ifdef OUL_PLUGINS
	g_string_append(str, "  <b>Plugins:</b> Enabled<br/>");
#else
	g_string_append(str, "  <b>Plugins:</b> Disabled<br/>");
#endif

#ifdef HAVE_SSL
	g_string_append(str, "  <b>SSL:</b> SSL support is present.<br/>");
#else
	g_string_append(str, "  <b>SSL:</b> SSL support was <b><i>NOT</i></b> compiled!<br/>");
#endif

/* This might be useful elsewhere too, but it is particularly useful for
 * debugging stuff known to be GTK+/Glib bugs on Windows */
g_string_append_printf(str, "  <b>GTK+ Runtime:</b> %u.%u.%u<br/>"
		"  <b>Glib Runtime:</b> %u.%u.%u<br/>",
		gtk_major_version, gtk_minor_version, gtk_micro_version,
		glib_major_version, glib_minor_version, glib_micro_version);


g_string_append(str, "<br/>  <b>Library Support</b><br/>");

#ifdef HAVE_CYRUS_SASL
	g_string_append_printf(str, "    <b>Cyrus SASL:</b> Enabled<br/>");
#else
	g_string_append_printf(str, "    <b>Cyrus SASL:</b> Disabled<br/>");
#endif

#ifdef HAVE_DBUS
	g_string_append_printf(str, "    <b>D-Bus:</b> Enabled<br/>");
#else
	g_string_append_printf(str, "    <b>D-Bus:</b> Disabled<br/>");
#endif

#ifdef HAVE_EVOLUTION_ADDRESSBOOK
	g_string_append_printf(str, "    <b>Evolution Addressbook:</b> Enabled<br/>");
#else
	g_string_append_printf(str, "    <b>Evolution Addressbook:</b> Disabled<br/>");
#endif


#ifdef HAVE_LIBGADU
	g_string_append(str, "    <b>Gadu-Gadu library (libgadu):</b> Enabled<br/>");
#else
	g_string_append(str, "    <b>Gadu-Gadu library (libgadu):</b> Disabled<br/>");
#endif

#ifdef USE_GTKSPELL
	g_string_append(str, "    <b>GtkSpell:</b> Enabled<br/>");
#else
	g_string_append(str, "    <b>GtkSpell:</b> Disabled<br/>");
#endif

#ifdef HAVE_GNUTLS
	g_string_append(str, "    <b>GnuTLS:</b> Enabled<br/>");
#else
	g_string_append(str, "    <b>GnuTLS:</b> Disabled<br/>");
#endif

#ifdef USE_GSTREAMER
	g_string_append(str, "    <b>GStreamer:</b> Enabled<br/>");
#else
	g_string_append(str, "    <b>GStreamer:</b> Disabled<br/>");
#endif

#ifdef ENABLE_MONO
	g_string_append(str, "    <b>Mono:</b> Enabled<br/>");
#else
	g_string_append(str, "    <b>Mono:</b> Disabled<br/>");
#endif

#ifdef HAVE_NETWORKMANAGER
	g_string_append(str, "    <b>NetworkManager:</b> Enabled<br/>");
#else
	g_string_append(str, "    <b>NetworkManager:</b> Disabled<br/>");
#endif

#ifdef HAVE_NSS
	g_string_append(str, "    <b>Network Security Services (NSS):</b> Enabled<br/>");
#else
	g_string_append(str, "    <b>Network Security Services (NSS):</b> Disabled<br/>");
#endif

if (oul_plugins_find_with_id("core-perl") != NULL)
	g_string_append(str, "    <b>Perl:</b> Enabled<br/>");
else
	g_string_append(str, "    <b>Perl:</b> Disabled<br/>");

#ifdef HAVE_STARTUP_NOTIFICATION
	g_string_append(str, "    <b>Startup Notification:</b> Enabled<br/>");
#else
	g_string_append(str, "    <b>Startup Notification:</b> Disabled<br/>");
#endif

if (oul_plugins_find_with_id("core-tcl") != NULL) {
	g_string_append(str, "    <b>Tcl:</b> Enabled<br/>");
#ifdef HAVE_TK
	g_string_append(str, "    <b>Tk:</b> Enabled<br/>");
#else
	g_string_append(str, "    <b>Tk:</b> Disabled<br/>");
#endif
} else {
	g_string_append(str, "    <b>Tcl:</b> Disabled<br/>");
	g_string_append(str, "    <b>Tk:</b> Disabled<br/>");
}

#ifdef USE_SM
	g_string_append(str, "    <b>X Session Management:</b> Enabled<br/>");
#else
	g_string_append(str, "    <b>X Session Management:</b> Disabled<br/>");
#endif

#ifdef USE_SCREENSAVER
	g_string_append(str, "    <b>XScreenSaver:</b> Enabled<br/>");
#else
	g_string_append(str, "    <b>XScreenSaver:</b> Disabled<br/>");
#endif

#ifdef LIBZEPHYR_EXT
	g_string_append(str, "    <b>Zephyr library (libzephyr):</b> External<br/>");
#else
	g_string_append(str, "    <b>Zephyr library (libzephyr):</b> Not External<br/>");
#endif

#ifdef ZEPHYR_USES_KERBEROS
	g_string_append(str, "    <b>Zephyr uses Kerberos:</b> Yes<br/>");
#else
	g_string_append(str, "    <b>Zephyr uses Kerberos:</b> No<br/>");
#endif


	/* End of not to be translated section */

	gtk_html_append_text(GTK_HTML(text), str->str, GTK_HTML_NO_SCROLL);
	g_string_free(str, TRUE);

	gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)), &iter);
	gtk_text_buffer_place_cursor(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)), &iter);

	/* Close Button */
	button = beasy_dialog_add_button(GTK_DIALOG(about), GTK_STOCK_CLOSE,
	                G_CALLBACK(destroy_about), about);

	g_signal_connect(G_OBJECT(about), "destroy",
					 G_CALLBACK(destroy_about), G_OBJECT(about));

	/* this makes the sizes not work? */
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);

	gtk_widget_show_all(about);
	gtk_window_present(GTK_WINDOW(about));
}

static gboolean
beasy_dialogs_ee(const char *ee)
{
	GtkWidget *window;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *img;
	gchar *norm = oul_strreplace(ee, "rocksmyworld", "");

	label = gtk_label_new(NULL);
	if (!strcmp(norm, "zilding"))
		gtk_label_set_markup(GTK_LABEL(label),
				     "<span weight=\"bold\" size=\"large\" foreground=\"oul\">Amazing!  Simply Amazing!</span>");
	else if (!strcmp(norm, "robflynn"))
		gtk_label_set_markup(GTK_LABEL(label),
				     "<span weight=\"bold\" size=\"large\" foreground=\"#1f6bad\">Pimpin\' Penguin Style! *Waddle Waddle*</span>");
	else if (!strcmp(norm, "flynorange"))
		gtk_label_set_markup(GTK_LABEL(label),
				      "<span weight=\"bold\" size=\"large\" foreground=\"blue\">You should be me.  I'm so cute!</span>");
	else if (!strcmp(norm, "ewarmenhoven"))
		gtk_label_set_markup(GTK_LABEL(label),
				     "<span weight=\"bold\" size=\"large\" foreground=\"orange\">Now that's what I like!</span>");
	else if (!strcmp(norm, "markster97"))
		gtk_label_set_markup(GTK_LABEL(label),
				     "<span weight=\"bold\" size=\"large\" foreground=\"brown\">Ahh, and excellent choice!</span>");
	else if (!strcmp(norm, "seanegn"))
		gtk_label_set_markup(GTK_LABEL(label),
				     "<span weight=\"bold\" size=\"large\" foreground=\"#009900\">Everytime you click my name, an angel gets its wings.</span>");
	else if (!strcmp(norm, "chipx86"))
		gtk_label_set_markup(GTK_LABEL(label),
				     "<span weight=\"bold\" size=\"large\" foreground=\"red\">This sunflower seed taste like pizza.</span>");
	else if (!strcmp(norm, "markdoliner"))
		gtk_label_set_markup(GTK_LABEL(label),
				     "<span weight=\"bold\" size=\"large\" foreground=\"#6364B1\">Hey!  I was in that tumbleweed!</span>");
	else if (!strcmp(norm, "lschiere"))
		gtk_label_set_markup(GTK_LABEL(label),
				     "<span weight=\"bold\" size=\"large\" foreground=\"gray\">I'm not anything.</span>");
	g_free(norm);

	if (strlen(gtk_label_get_label(GTK_LABEL(label))) <= 0)
		return FALSE;

	window = gtk_dialog_new_with_buttons(BEASY_ALERT_TITLE, NULL, 0, GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG(window), GTK_RESPONSE_OK);
	g_signal_connect(G_OBJECT(window), "response", G_CALLBACK(gtk_widget_destroy), NULL);

	gtk_container_set_border_width (GTK_CONTAINER(window), BEASY_HIG_BOX_SPACE);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gtk_dialog_set_has_separator(GTK_DIALOG(window), FALSE);
	gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(window)->vbox), BEASY_HIG_BORDER);
	gtk_container_set_border_width (GTK_CONTAINER(GTK_DIALOG(window)->vbox), BEASY_HIG_BOX_SPACE);

	hbox = gtk_hbox_new(FALSE, BEASY_HIG_BORDER);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(window)->vbox), hbox);
	img = gtk_image_new_from_stock(BEASY_STOCK_DIALOG_COOL, gtk_icon_size_from_name(BEASY_ICON_SIZE_TANGO_HUGE));
	gtk_box_pack_start(GTK_BOX(hbox), img, FALSE, FALSE, 0);

	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	gtk_widget_show_all(window);
	return TRUE;
}

