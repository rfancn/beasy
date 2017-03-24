/**
 * @file gtkpluginpref.c GTK+ Plugin preferences
 * @ingroup beasy
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "internal.h"

#include "debug.h"
#include "pluginpref.h"
#include "prefs.h"

#include "gtkhtml.h"
#include "gtkpluginpref.h"
#include "gtkprefs.h"
#include "gtkutils.h"

static gboolean
entry_cb(GtkWidget *entry, gpointer data) {
	char *pref = data;

	oul_prefs_set_string(pref, gtk_entry_get_text(GTK_ENTRY(entry)));

	return FALSE;
}


static void
html_cb(GtkTextBuffer *buffer, gpointer data)
{
	char *pref;
	char *text;
	GtkHtml *html = data;

	pref = g_object_get_data(G_OBJECT(html), "pref-key");
	g_return_if_fail(pref);

	text = gtk_html_get_markup(html);
	oul_prefs_set_string(pref, text);
	g_free(text);
}

static void
html_format_cb(GtkHtml *html, GtkHtmlButtons buttons, gpointer data)
{
	html_cb(gtk_text_view_get_buffer(GTK_TEXT_VIEW(html)), data);
}

static void
make_string_pref(GtkWidget *parent, OulPluginPref *pref, GtkSizeGroup *sg) {
	GtkWidget *box, *gtk_label, *entry;
	const gchar *pref_name;
	const gchar *pref_label;
	OulStringFormatType format;

	pref_name = oul_plugin_pref_get_name(pref);
	pref_label = oul_plugin_pref_get_label(pref);
	format = oul_plugin_pref_get_format_type(pref);

	switch(oul_plugin_pref_get_type(pref)) {
		case OUL_PLUGIN_PREF_CHOICE:
			gtk_label = beasy_prefs_dropdown_from_list(parent, pref_label,
											  OUL_PREF_STRING, pref_name,
											  oul_plugin_pref_get_choices(pref));
			gtk_misc_set_alignment(GTK_MISC(gtk_label), 0, 0.5);

			if(sg)
				gtk_size_group_add_widget(sg, gtk_label);

			break;
		case OUL_PLUGIN_PREF_NONE:
		default:
			if (format == OUL_STRING_FORMAT_TYPE_NONE)
			{				
				entry = gtk_entry_new();
				gtk_entry_set_text(GTK_ENTRY(entry), oul_prefs_get_string(pref_name));
				gtk_entry_set_max_length(GTK_ENTRY(entry),
									 oul_plugin_pref_get_max_length(pref));
				if (oul_plugin_pref_get_masked(pref))
				{
					gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
					if (gtk_entry_get_invisible_char(GTK_ENTRY(entry)) == '*')
						gtk_entry_set_invisible_char(GTK_ENTRY(entry), BEASY_INVISIBLE_CHAR);
				}
				g_signal_connect(G_OBJECT(entry), "changed",
								 G_CALLBACK(entry_cb),
								 (gpointer)pref_name);
				beasy_add_widget_to_vbox(GTK_BOX(parent), pref_label, sg, entry, TRUE, NULL);
			}
			else
			{
				GtkWidget *hbox;
				GtkWidget *spacer;
				GtkWidget *html;
				GtkWidget *toolbar;
				GtkWidget *frame;

				box = gtk_vbox_new(FALSE, BEASY_HIG_BOX_SPACE);

				gtk_widget_show(box);
				gtk_box_pack_start(GTK_BOX(parent), box, FALSE, FALSE, 0);

				gtk_label = gtk_label_new_with_mnemonic(pref_label);
				gtk_misc_set_alignment(GTK_MISC(gtk_label), 0, 0.5);
				gtk_widget_show(gtk_label);
				gtk_box_pack_start(GTK_BOX(box), gtk_label, FALSE, FALSE, 0);

				if(sg)
					gtk_size_group_add_widget(sg, gtk_label);

				hbox = gtk_hbox_new(FALSE, BEASY_HIG_BOX_SPACE);
				gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);
				gtk_widget_show(hbox);

				spacer = gtk_label_new("    ");
				gtk_box_pack_start(GTK_BOX(hbox), spacer, FALSE, FALSE, 0);
				gtk_widget_show(spacer);

				frame = beasy_create_html(TRUE, &html, NULL);
			
				gtk_html_append_text(GTK_HTML(html), oul_prefs_get_string(pref_name),
						(format & OUL_STRING_FORMAT_TYPE_MULTILINE) ? 0 : GTK_HTML_NO_NEWLINE);
				gtk_label_set_mnemonic_widget(GTK_LABEL(gtk_label), html);
				gtk_widget_show_all(frame);
				g_object_set_data(G_OBJECT(html), "pref-key", (gpointer)pref_name);
				g_signal_connect(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(html))),
								"changed", G_CALLBACK(html_cb), html);
				g_signal_connect(G_OBJECT(html),
								"format_function_toggle", G_CALLBACK(html_format_cb), html);
				gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
			}

			break;
	}
}

static void
make_int_pref(GtkWidget *parent, OulPluginPref *pref, GtkSizeGroup *sg) {
	GtkWidget *gtk_label;
	const gchar *pref_name;
	const gchar *pref_label;
	gint max, min;

	pref_name = oul_plugin_pref_get_name(pref);
	pref_label = oul_plugin_pref_get_label(pref);

	switch(oul_plugin_pref_get_type(pref)) {
		case OUL_PLUGIN_PREF_CHOICE:
			gtk_label = beasy_prefs_dropdown_from_list(parent, pref_label,
					OUL_PREF_INT, pref_name, oul_plugin_pref_get_choices(pref));
			gtk_misc_set_alignment(GTK_MISC(gtk_label), 0, 0.5);

			if(sg)
				gtk_size_group_add_widget(sg, gtk_label);

			break;
		case OUL_PLUGIN_PREF_NONE:
		default:
			oul_plugin_pref_get_bounds(pref, &min, &max);
			beasy_prefs_labeled_spin_button(parent, pref_label,
					pref_name, min, max, sg);
			break;
	}
}


static void
make_info_pref(GtkWidget *parent, OulPluginPref *pref) {
	GtkWidget *gtk_label = gtk_label_new(oul_plugin_pref_get_label(pref));
	gtk_misc_set_alignment(GTK_MISC(gtk_label), 0, 0);
	gtk_label_set_line_wrap(GTK_LABEL(gtk_label), TRUE);
	gtk_box_pack_start(GTK_BOX(parent), gtk_label, FALSE, FALSE, 0);
	gtk_widget_show(gtk_label);
}


GtkWidget *
beasy_plugin_pref_create_frame(OulPluginPrefFrame *frame) {
	GtkWidget *ret, *parent;
	GtkSizeGroup *sg;
	GList *pp;

	g_return_val_if_fail(frame, NULL);

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	parent = ret = gtk_vbox_new(FALSE, 16);
	gtk_container_set_border_width(GTK_CONTAINER(ret), BEASY_HIG_BORDER);
	gtk_widget_show(ret);

	for(pp = oul_plugin_pref_frame_get_prefs(frame); pp != NULL; pp = pp->next)
	{
		OulPluginPref *pref = (OulPluginPref *)pp->data;

		const char *name = oul_plugin_pref_get_name(pref);
		const char *label = oul_plugin_pref_get_label(pref);

		if(name == NULL) {
			if(label == NULL)
				continue;

			if(label != NULL)
				oul_debug_info("beasy_plugin_pref_create_frame", "%s.\n", label);

			if(oul_plugin_pref_get_type(pref) == OUL_PLUGIN_PREF_INFO) {
				make_info_pref(parent, pref);
			} else {
				parent = beasy_make_frame(ret, label);
				gtk_widget_show(parent);
			}

			continue;
		}

		switch(oul_prefs_get_type(name)) {
			case OUL_PREF_BOOLEAN:
				beasy_prefs_checkbox(label, name, parent);
				break;
			case OUL_PREF_INT:
				make_int_pref(parent, pref, sg);
				break;
			case OUL_PREF_STRING:
				make_string_pref(parent, pref, sg);
				break;
			default:
				break;
		}
	}

	g_object_unref(sg);

	return ret;
}
