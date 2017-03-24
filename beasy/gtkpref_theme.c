#include "internal.h"

#include <gtk/gtk.h>

#include "gtkntf_theme.h"
#include "gtkpref_theme.h"
#include "gtkutils.h"

enum {
	GTKNTF_THEME_COL_FILE = 0,
	GTKNTF_THEME_COL_LOADED,
	GTKNTF_THEME_COL_NAME,
	GTKNTF_THEME_COL_VERSION,
	GTKNTF_THEME_COL_SUMMARY,
	GTKNTF_THEME_COL_DESCRIPTION,
	GTKNTF_THEME_COL_AUTHOR,
	GTKNTF_THEME_COL_WEBSITE,
	GTKNTF_THEME_COL_SUPPORTS
};


struct GtkNtfThemeListData {
	GtkWidget *tree;
	GtkListStore *store;
	GtkWidget *theme_new;
	GtkWidget *theme_edit;
	GtkWidget *theme_delete;
	GtkWidget *theme_copy;
	GtkWidget *theme_refresh;
	GtkWidget *theme_get_more;
};

struct ThemeInfoPane {
	GtkWidget *theme_name;
	GtkWidget *theme_version;
	GtkWidget *theme_description;
	GtkWidget *theme_author;
	GtkWidget *theme_website;
	GtkWidget *theme_supports;
	GtkWidget *theme_filename;
};

static struct GtkNtfThemeListData theme_data;
static struct ThemeInfoPane theme_info_pane;

static gint
theme_sort_loaded(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,
				  gpointer data)
{
	gboolean a_loaded = FALSE, b_loaded = FALSE;

	gtk_tree_model_get(model, a, GTKNTF_THEME_COL_LOADED, &a_loaded, -1);
	gtk_tree_model_get(model, b, GTKNTF_THEME_COL_LOADED, &b_loaded, -1);

	if(a_loaded && !b_loaded)
		return 1;
	else if(!a_loaded && b_loaded)
		return -1;
	else
		return 0;
}

static gint
theme_sort_name(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,
				gpointer data)
{
	gchar *a_name = NULL, *b_name = NULL;
	gchar *a_ckey = NULL, *b_ckey = NULL;
	gint ret = 0;

	gtk_tree_model_get(model, a, GTKNTF_THEME_COL_NAME, &a_name, -1);
	gtk_tree_model_get(model, b, GTKNTF_THEME_COL_NAME, &b_name, -1);

	if(a_name && !b_name)
		return 1;
	else if(!a_name && b_name)
		return -1;

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
theme_sort_summary(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,
				   gpointer data)
{
	gchar *a_summ = NULL, *b_summ = NULL;
	gchar *a_ckey = NULL, *b_ckey = NULL;
	gint ret = 0;

	gtk_tree_model_get(model, a, GTKNTF_THEME_COL_SUMMARY, &a_summ, -1);
	gtk_tree_model_get(model, b, GTKNTF_THEME_COL_SUMMARY, &b_summ, -1);

	if(!a_summ && !b_summ)
		return 0;

	if(a_summ && !b_summ)
		return 1;
	else if(!a_summ && b_summ)
		return -1;

	a_ckey = g_utf8_collate_key(a_summ, g_utf8_strlen(a_summ, -1));
	b_ckey = g_utf8_collate_key(b_summ, g_utf8_strlen(b_summ, -1));

	g_free(a_summ);
	g_free(b_summ);

	ret = strcmp(a_ckey, b_ckey);

	g_free(a_ckey);
	g_free(b_ckey);

	return ret;
}



static GtkListStore *
create_theme_store() {
	GtkNtfTheme *theme;
	GtkNtfThemeInfo *info;
	GtkListStore *store;
	GtkTreeSortable *sortable;
	GtkTreeIter iter;
	GList *l;
	gchar *supports;
	gboolean loaded = FALSE, destroy;

	gtkntf_themes_unprobe();
	gtkntf_themes_probe();
	oul_debug_info("GTK Notify", "probes refreshed\n");

	store = gtk_list_store_new(9, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING,
							   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
							   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	for(l = gtkntf_themes_get_all(); l; l = l->next) {
		gtk_list_store_append(store, &iter);

		loaded = gtkntf_theme_is_loaded(l->data);

		/* set the filename and if it's loaded */
		gtk_list_store_set(store, &iter,
						   GTKNTF_THEME_COL_FILE, l->data,
						   GTKNTF_THEME_COL_LOADED, loaded,
						   -1);

		/* find the theme */
		if(loaded) {
			theme = gtkntf_theme_find_theme_by_filename(l->data);
			destroy = FALSE;
		} else {
			theme = gtkntf_theme_new_from_file(l->data);
			destroy = TRUE;
		}

		info = gtkntf_theme_get_theme_info(theme);
		supports = gtkntf_theme_get_supported_notifications(theme);

		gtk_list_store_set(store, &iter,
						   GTKNTF_THEME_COL_NAME, gtkntf_theme_info_get_name(info),
						   GTKNTF_THEME_COL_VERSION, gtkntf_theme_info_get_version(info),
						   GTKNTF_THEME_COL_SUMMARY, gtkntf_theme_info_get_summary(info),
						   GTKNTF_THEME_COL_DESCRIPTION, gtkntf_theme_info_get_description(info),
						   GTKNTF_THEME_COL_AUTHOR, gtkntf_theme_info_get_author(info),
						   GTKNTF_THEME_COL_WEBSITE, gtkntf_theme_info_get_website(info),
						   GTKNTF_THEME_COL_SUPPORTS, supports,
						   -1);

		g_free(supports);

		if(destroy)
			gtkntf_theme_destory(theme);
	}

	sortable = GTK_TREE_SORTABLE(store);
	gtk_tree_sortable_set_sort_func(sortable, GTKNTF_THEME_COL_LOADED,
									theme_sort_loaded, NULL, NULL);
	gtk_tree_sortable_set_sort_func(sortable, GTKNTF_THEME_COL_NAME,
									theme_sort_name, NULL, NULL);
	gtk_tree_sortable_set_sort_func(sortable, GTKNTF_THEME_COL_SUMMARY,
									theme_sort_summary, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(sortable, GTKNTF_THEME_COL_NAME,
										 GTK_SORT_ASCENDING);

	return store;
}

static void theme_list_new_cb(){};
static void theme_list_edit_cb(){};
static void theme_list_delete_cb(){};
static void theme_list_refresh_cb(){};


static gboolean
theme_list_clicked_cb(GtkWidget *w, GdkEventButton *e, gpointer data) {
	if(e->button == 3) {
		GtkWidget *menu;
		GtkTreeSelection *sel;
		GtkTreeModel *model;
		GtkTreeIter iter;

		menu = gtk_menu_new();

		beasy_new_item_from_stock(menu, _("New"), GTK_STOCK_NEW,
								 G_CALLBACK(theme_list_new_cb), NULL,
								 0, 0, NULL);

		sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(theme_data.tree));
		if(gtk_tree_selection_get_selected(sel, &model, &iter)) {
			gchar *filename;

			gtk_tree_model_get(model, &iter,
							   GTKNTF_THEME_COL_FILE, &filename, -1);

			if(!gtkntf_file_access(filename, W_OK)) {
				beasy_new_item_from_stock(menu, _("Edit"), GTK_STOCK_PREFERENCES,
										 G_CALLBACK(theme_list_edit_cb),
										 sel, 0, 0, NULL);

				beasy_new_item_from_stock(menu, _("Delete"), GTK_STOCK_DELETE,
										 G_CALLBACK(theme_list_delete_cb),
										 sel, 0, 0, NULL);
			}

			if(filename)
				g_free(filename);
		}

		beasy_separator(menu);

		beasy_new_item_from_stock(menu, _("Refresh"), GTK_STOCK_REFRESH,
								 G_CALLBACK(theme_list_refresh_cb), NULL,
								 0, 0, NULL);

		gtk_widget_show_all(menu);

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3,
					   gtk_get_current_event_time());

		return TRUE;
	}

	return FALSE;
}

static void
theme_list_refresh() {
	if(theme_data.tree) {
		GtkTreeSelection *sel;
		GtkTreeIter iter;

		gtk_tree_view_set_model(GTK_TREE_VIEW(theme_data.tree), NULL);
		gtk_list_store_clear(theme_data.store);
		g_object_unref(G_OBJECT(theme_data.store));

		theme_data.store = create_theme_store();
		gtk_tree_view_set_model(GTK_TREE_VIEW(theme_data.tree),
								GTK_TREE_MODEL(theme_data.store));

		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(theme_data.store), &iter);
		sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(GTK_TREE_VIEW(theme_data.tree)));
		gtk_tree_selection_select_iter(sel, &iter);
	}
}


static void
theme_install_theme(char *path, char *extn) {
#ifndef _WIN32
	gchar *command, *escaped;
#endif
	gchar *destdir;
	gchar *tail;

	/* Just to be safe */
	g_strchomp(path);

	/* I dont know what you are, get out of here */
	if (extn != NULL)
		tail = extn;
	else if ((tail = strrchr(path, '.')) == NULL)
		return;

	destdir = g_build_filename(oul_user_dir(), "beasy", "themes", NULL);

	/* We'll check this just to make sure. This also lets us do something
	 * different on other platforms, if need be */
	if (!g_ascii_strcasecmp(tail, ".gz") || !g_ascii_strcasecmp(tail, ".tgz")) {
		escaped = g_shell_quote(path);
		command = g_strdup_printf("tar > /dev/null xzf %s -C %s", escaped, destdir);
		g_free(escaped);
	}
	else {
		g_free(destdir);
		return;
	}

	/* Fire! */
	system(command);

	g_free(command);
	g_free(destdir);

	theme_list_refresh();
}



static void
theme_dnd_recv(GtkWidget *widget, GdkDragContext *dc, guint x, guint y, GtkSelectionData *sd,
				guint info, guint t, gpointer data) {
	gchar *name = (gchar *)sd->data;

	if ((sd->length >= 0) && (sd->format == 8)) {
		/* Well, it looks like the drag event was cool.
		 * Let's do something with it */

		if (!g_ascii_strncasecmp(name, "file://", 7)) {
			GError *converr = NULL;
			gchar *tmp;
			/* It looks like we're dealing with a local file. Let's
			 * just untar it in the right place */
			if(!(tmp = g_filename_from_uri(name, NULL, &converr))) {
				oul_debug_error("guifications", "theme dnd %s\n",
						   (converr ? converr->message :
							"g_filename_from_uri error"));
				return;
			}
			theme_install_theme(tmp, NULL);
			g_free(tmp);
		} else if (!g_ascii_strncasecmp(name, "http://", 7)) {
			/* Oo, a web drag and drop. This is where things
			 * will start to get interesting */
			gchar *tail;

			if ((tail = strrchr(name, '.')) == NULL)
				return;

			/* We'll check this just to make sure. This also lets us do something different on
			 * other platforms, if need be */
			/* Q: shouldn't tgz be tail? */
			/* A: no. */
			#if 0
			Oul_util_fetch_url(name, TRUE, NULL, FALSE, theme_got_url, ".tgz");
			#endif
		}

		gtk_drag_finish(dc, TRUE, FALSE, t);
	}

	gtk_drag_finish(dc, FALSE, FALSE, t);
}

static void
theme_load_cb(GtkCellRendererToggle *renderer, gchar *path, gpointer data) {
	GtkTreeIter iter;
	gchar *filename = NULL;
	gboolean loaded = FALSE;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(theme_data.store),
										&iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(theme_data.store), &iter,
					   GTKNTF_THEME_COL_LOADED, &loaded,
					   GTKNTF_THEME_COL_FILE, &filename,
					   -1);

	if(!loaded) {
		gtkntf_theme_load(filename);
	} else {
		GtkNtfTheme *theme;

		theme = gtkntf_theme_find_theme_by_filename(filename);
		if(theme)
			gtkntf_theme_unload(theme);
	}

	gtk_list_store_set(theme_data.store, &iter,
					   GTKNTF_THEME_COL_LOADED, !loaded,
					   -1);

	if(filename)
		g_free(filename);

	gtkntf_themes_save_loaded();
}


static void
theme_list_selection_cb(GtkTreeSelection *selection, gpointer data) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *name = NULL, *version = NULL, *description = NULL;
	gchar *author = NULL, *website = NULL, *filename = NULL;
	gchar *supports = NULL;

	if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get(model, &iter,
						   GTKNTF_THEME_COL_NAME, &name,
						   GTKNTF_THEME_COL_VERSION, &version,
						   GTKNTF_THEME_COL_DESCRIPTION, &description,
						   GTKNTF_THEME_COL_AUTHOR, &author,
						   GTKNTF_THEME_COL_WEBSITE, &website,
						   GTKNTF_THEME_COL_FILE, &filename,
						   GTKNTF_THEME_COL_SUPPORTS, &supports,
						   -1);

		if(filename) {
			if(!gtkntf_file_access(filename, W_OK)) {
				gtk_widget_set_sensitive(theme_data.theme_edit, TRUE);
				gtk_widget_set_sensitive(theme_data.theme_delete, TRUE);
			} else {
				gtk_widget_set_sensitive(theme_data.theme_edit, FALSE);
				gtk_widget_set_sensitive(theme_data.theme_delete, FALSE);
			}
		}

		gtk_widget_set_sensitive(theme_data.theme_copy, TRUE);
	} else {
		gtk_widget_set_sensitive(theme_data.theme_copy, FALSE);
	}

	gtk_label_set_text(GTK_LABEL(theme_info_pane.theme_name), name);
	gtk_label_set_text(GTK_LABEL(theme_info_pane.theme_version), version);
	gtk_label_set_text(GTK_LABEL(theme_info_pane.theme_description), description);
	gtk_label_set_text(GTK_LABEL(theme_info_pane.theme_author), author);
	gtk_label_set_text(GTK_LABEL(theme_info_pane.theme_website), website);
	gtk_label_set_text(GTK_LABEL(theme_info_pane.theme_supports), supports);
	gtk_label_set_text(GTK_LABEL(theme_info_pane.theme_filename), filename);

	g_free(name);
	g_free(version);
	g_free(description);
	g_free(author);
	g_free(website);
	g_free(supports);
	g_free(filename);
}

static void
theme_list_copy_cb(GtkWidget *w, gpointer data) {
	GtkNtfTheme *theme;
	GtkNtfThemeInfo *info;
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;
	const gchar *oldname;
	gchar *filename, *newname, *fullname, *oldpath, *newpath, *dir;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(theme_data.tree));

	if(gtk_tree_selection_get_selected(sel, &model, &iter))
		gtk_tree_model_get(model, &iter, GTKNTF_THEME_COL_FILE, &filename, -1);

	if(!filename)
		return;

	theme = gtkntf_theme_new_from_file(filename);
	if(!theme)
		return;

	info = gtkntf_theme_get_theme_info(theme);
	oldname = gtkntf_theme_info_get_name(info);
	if(oldname)
		newname = g_strdup_printf("%s (copy)", oldname);
	else
		newname = g_strdup("untitled (copy)");
	gtkntf_theme_info_set_name(info, newname);
	g_free(newname);

	dir = gtkntf_theme_info_strip_name(info);
	if(!dir) {
		gtkntf_theme_destory(theme);
		return;
	}

	newpath = g_build_filename(oul_user_dir(), "themes", dir, NULL);
	g_free(dir);
	oul_build_dir(newpath, S_IRUSR | S_IWUSR | S_IXUSR);

	fullname = g_build_filename(newpath, "theme.xml", NULL);

	/* we copy everything first, and then rewrite theme.xml.  Yeah it's not ideal but it
	 * keeps the code simple.
	 */
	oldpath = g_path_get_dirname(filename);
	gtkntf_file_copy_directory(oldpath, newpath);
	g_free(oldpath);
	g_free(newpath);

	gtkntf_theme_save_to_file(theme, fullname);
	g_free(fullname);

	theme_list_refresh();
}


static void
theme_list_get_more_cb(GtkWidget *w, gpointer data) {
	oul_notify_uri(NULL, "http://rfan.512j.com");
}


static void
make_theme_list(GtkBox *parent)
{
	GtkWidget *hbox, *sw;
	GtkCellRenderer *renderer;
	GtkTreeSelection *sel;
	GtkTreeViewColumn *col;
	GtkTargetEntry te[3] = {{"text/plain", 0, 0},{"text/uri-list", 0, 1},{"STRING", 0, 2}};

	/* scrolled window and tree */
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
								   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_IN);
	gtk_box_pack_start(parent, sw, TRUE, TRUE, 0);

	theme_data.store = create_theme_store();
	theme_data.tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(theme_data.store));
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(theme_data.tree), TRUE);
	gtk_widget_set_size_request(theme_data.tree, -1, 150);
	gtk_container_add(GTK_CONTAINER(sw), theme_data.tree);
	g_signal_connect(G_OBJECT(theme_data.tree), "button-press-event",
					 G_CALLBACK(theme_list_clicked_cb), NULL);

	gtk_drag_dest_set(theme_data.tree, GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_DROP, te,
					  sizeof(te) / sizeof(GtkTargetEntry) , GDK_ACTION_COPY | GDK_ACTION_MOVE);
	g_signal_connect(G_OBJECT(theme_data.tree), "drag_data_received", G_CALLBACK(theme_dnd_recv), theme_data.store);

	renderer = gtk_cell_renderer_toggle_new();
	g_signal_connect(G_OBJECT(renderer), "toggled",
					 G_CALLBACK(theme_load_cb), NULL);
	col = gtk_tree_view_column_new_with_attributes(_("Loaded"), renderer,
												   "active", GTKNTF_THEME_COL_LOADED,
												   NULL);
	gtk_tree_view_column_set_sort_column_id(col, GTKNTF_THEME_COL_LOADED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(theme_data.tree), col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Name"), renderer,
												   "text", GTKNTF_THEME_COL_NAME,
												   NULL);
	gtk_tree_view_column_set_sort_column_id(col, GTKNTF_THEME_COL_NAME);
	gtk_tree_view_append_column(GTK_TREE_VIEW(theme_data.tree), col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Summary"), renderer,
												   "text", GTKNTF_THEME_COL_SUMMARY,
												   NULL);
	gtk_tree_view_column_set_sort_column_id(col, GTKNTF_THEME_COL_SUMMARY);
	gtk_tree_view_append_column(GTK_TREE_VIEW(theme_data.tree), col);

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(theme_data.tree));
	g_signal_connect(G_OBJECT(sel), "changed",
					 G_CALLBACK(theme_list_selection_cb), NULL);

	/* the new, edit, delete, refresh buttons and get more buttons */
	hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

	theme_data.theme_new = gtk_button_new_from_stock(GTK_STOCK_NEW);
	gtk_button_set_relief(GTK_BUTTON(theme_data.theme_new), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(theme_data.theme_new), "clicked",
					 G_CALLBACK(theme_list_new_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), theme_data.theme_new, FALSE, FALSE, 0);

	theme_data.theme_edit =	beasy_pixbuf_button_from_stock(_("_Edit"),
												  GTK_STOCK_PREFERENCES,
												  BEASY_BUTTON_HORIZONTAL);
	gtk_button_set_relief(GTK_BUTTON(theme_data.theme_edit), GTK_RELIEF_NONE);
	gtk_widget_set_sensitive(theme_data.theme_edit, FALSE);
	g_signal_connect(G_OBJECT(theme_data.theme_edit), "clicked",
					 G_CALLBACK(theme_list_edit_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), theme_data.theme_edit, FALSE, FALSE, 0);

	theme_data.theme_delete = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_button_set_relief(GTK_BUTTON(theme_data.theme_delete), GTK_RELIEF_NONE);
	gtk_widget_set_sensitive(theme_data.theme_delete, FALSE);
	g_signal_connect(G_OBJECT(theme_data.theme_delete), "clicked",
					 G_CALLBACK(theme_list_delete_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), theme_data.theme_delete, FALSE, FALSE, 0);

	theme_data.theme_copy = gtk_button_new_from_stock(GTK_STOCK_COPY);
	gtk_button_set_relief(GTK_BUTTON(theme_data.theme_copy), GTK_RELIEF_NONE);
	gtk_widget_set_sensitive(theme_data.theme_copy, FALSE);
	g_signal_connect(G_OBJECT(theme_data.theme_copy), "clicked",
					 G_CALLBACK(theme_list_copy_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), theme_data.theme_copy, FALSE, FALSE, 0);

	theme_data.theme_refresh = gtk_button_new_from_stock(GTK_STOCK_REFRESH);
	gtk_button_set_relief(GTK_BUTTON(theme_data.theme_refresh), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(theme_data.theme_refresh), "clicked",
					 G_CALLBACK(theme_list_refresh_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), theme_data.theme_refresh, FALSE, FALSE, 0);

	theme_data.theme_get_more = 
					beasy_pixbuf_button_from_stock(_("_Get More"),
												  GTK_STOCK_JUMP_TO,
												  BEASY_BUTTON_HORIZONTAL);
	gtk_button_set_relief(GTK_BUTTON(theme_data.theme_get_more), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(theme_data.theme_get_more), "clicked",
					 G_CALLBACK(theme_list_get_more_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), theme_data.theme_get_more, FALSE, FALSE, 0);
	
	gtk_widget_show_all(sw);
}


static GtkWidget *
make_bold_label(const gchar *text, GtkSizeGroup *sg) {
	GtkWidget *label;
	gchar *escaped, *markup;

	escaped = g_markup_escape_text(text, strlen(text));
	markup = g_strdup_printf("<b>%s:</b>", escaped);
	g_free(escaped);

	label = gtk_label_new(NULL);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), markup);
	g_free(markup);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);

	if(sg)
		gtk_size_group_add_widget(sg, label);

	return label;
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

GtkWidget *
beasy_theme_prefpage_get(void) {
	GtkWidget *ret, *sw, *vp, *vbox, *hbox, *label;
	GtkSizeGroup *sg;

	ret = gtk_vbox_new(FALSE, BEASY_HIG_CAT_SPACE);
	gtk_container_set_border_width (GTK_CONTAINER (ret), BEASY_HIG_BORDER);

	make_theme_list(GTK_BOX(ret));

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
								   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(ret), sw, TRUE, TRUE, 0);

	vp = gtk_viewport_new(NULL, NULL);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(vp), GTK_SHADOW_NONE);
	gtk_container_set_border_width(GTK_CONTAINER(vp), 4);
	gtk_container_add(GTK_CONTAINER(sw), vp);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vp), vbox);

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	/* name */
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	label = make_bold_label(_("Name"), sg);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	theme_info_pane.theme_name = make_label(NULL, NULL);
	gtk_label_set_line_wrap(GTK_LABEL(theme_info_pane.theme_name), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), theme_info_pane.theme_name,
					   FALSE, FALSE, 0);

	/* version */
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	label = make_bold_label(_("Version"), sg);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	theme_info_pane.theme_version = make_label(NULL, NULL);
	gtk_label_set_line_wrap(GTK_LABEL(theme_info_pane.theme_version), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), theme_info_pane.theme_version,
					   FALSE, FALSE, 0);

	/* description */
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	label = make_bold_label(_("Description"), sg);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	theme_info_pane.theme_description = make_label(NULL, NULL);
	gtk_label_set_line_wrap(GTK_LABEL(theme_info_pane.theme_description), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), theme_info_pane.theme_description,
					   FALSE, FALSE, 0);

	/* author */
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	label = make_bold_label(_("Author"), sg);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	theme_info_pane.theme_author = make_label(NULL, NULL);
	gtk_label_set_line_wrap(GTK_LABEL(theme_info_pane.theme_author), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), theme_info_pane.theme_author,
					   FALSE, FALSE, 0);

	/* website */
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	label = make_bold_label(_("Website"), sg);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	theme_info_pane.theme_website = make_label(NULL, NULL);
	gtk_label_set_line_wrap(GTK_LABEL(theme_info_pane.theme_website), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), theme_info_pane.theme_website,
					   FALSE, FALSE, 0);

	/* supports */
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	label = make_bold_label(_("Supports"), sg);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	theme_info_pane.theme_supports = make_label(NULL, NULL);
	gtk_label_set_line_wrap(GTK_LABEL(theme_info_pane.theme_supports), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), theme_info_pane.theme_supports,
					   FALSE, FALSE, 0);

	/* filename */
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	label = make_bold_label(_("Filename"), sg);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	theme_info_pane.theme_filename = make_label(NULL, NULL);
	gtk_label_set_line_wrap(GTK_LABEL(theme_info_pane.theme_filename), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), theme_info_pane.theme_filename,
					   FALSE, FALSE, 0);

	gtk_widget_show_all(ret);
	
	return ret;
}

