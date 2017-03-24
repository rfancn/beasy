#include "internal.h"
#include "beasy.h"

# include <X11/Xlib.h>
# ifdef small
#  undef small
# endif

#ifdef USE_GTKSPELL
# include <gtkspell/gtkspell.h>
#endif

#include <gdk/gdkkeysyms.h>

#include "beasystock.h"
#include "debug.h"
#include "notify.h"
#include "prefs.h"
#include "signals.h"
#include "util.h"
#include "imgstore.h"
#include "status.h"
#include "savedstatuses.h"

#include "gtkhtml.h"
#include "gtkutils.h"

typedef struct {
	GtkWidget *menu;
	gint default_item;
} AopMenu;

static guint accels_save_timer = 0;

static gboolean
url_clicked_idle_cb(gpointer data)
{
	oul_notify_uri(NULL, data);
	g_free(data);
	return FALSE;
}

static void
url_clicked_cb(GtkWidget *w, const char *uri)
{
	g_idle_add(url_clicked_idle_cb, g_strdup(uri));
}

static
void beasy_window_init(GtkWindow *wnd, const char *title, guint border_width, const char *role, gboolean resizable)
{
	if (title)
		gtk_window_set_title(wnd, title);

	gtk_container_set_border_width(GTK_CONTAINER(wnd), border_width);
	if (role)
		gtk_window_set_role(wnd, role);
	gtk_window_set_resizable(wnd, resizable);
}

GtkWidget *
beasy_create_window(const char *title, guint border_width, const char *role, gboolean resizable)
{
	GtkWindow *wnd = NULL;

	wnd = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	beasy_window_init(wnd, title, border_width, role, resizable);

	return GTK_WIDGET(wnd);
}

GtkWidget *
beasy_create_dialog(const char *title, guint border_width, const char *role, gboolean resizable)
{
	GtkWindow *wnd = NULL;

	wnd = GTK_WINDOW(gtk_dialog_new());
	beasy_window_init(wnd, title, border_width, role, resizable);
	g_object_set(G_OBJECT(wnd), "has-separator", FALSE, NULL);

	return GTK_WIDGET(wnd);
}

GtkWidget *
beasy_dialog_get_vbox_with_properties(GtkDialog *dialog, gboolean homogeneous, gint spacing)
{
	GtkBox *vbox = GTK_BOX(GTK_DIALOG(dialog)->vbox);
	gtk_box_set_homogeneous(vbox, homogeneous);
	gtk_box_set_spacing(vbox, spacing);
	return GTK_WIDGET(vbox);
}

GtkWidget *beasy_dialog_get_vbox(GtkDialog *dialog)
{
	return GTK_DIALOG(dialog)->vbox;
}

GtkWidget *beasy_dialog_get_action_area(GtkDialog *dialog)
{
	return GTK_DIALOG(dialog)->action_area;
}

GtkWidget *beasy_dialog_add_button(GtkDialog *dialog, const char *label,
		GCallback callback, gpointer callbackdata)
{
	GtkWidget *button = gtk_button_new_from_stock(label);
	GtkWidget *bbox = beasy_dialog_get_action_area(dialog);
	gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);
	if (callback)
		g_signal_connect(G_OBJECT(button), "clicked", callback, callbackdata);
	gtk_widget_show(button);
	return button;
}

void
beasy_set_sensitive_if_input(GtkWidget *entry, GtkWidget *dialog)
{
	const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
	gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK,
									  (*text != '\0'));
}

void
beasy_toggle_sensitive(GtkWidget *widget, GtkWidget *to_toggle)
{
	gboolean sensitivity;

	if (to_toggle == NULL)
		return;

	sensitivity = GTK_WIDGET_IS_SENSITIVE(to_toggle);

	gtk_widget_set_sensitive(to_toggle, !sensitivity);
}

void
beasy_toggle_sensitive_array(GtkWidget *w, GPtrArray *data)
{
	gboolean sensitivity;
	gpointer element;
	int i;

	for (i=0; i < data->len; i++) {
		element = g_ptr_array_index(data,i);
		if (element == NULL)
			continue;

		sensitivity = GTK_WIDGET_IS_SENSITIVE(element);

		gtk_widget_set_sensitive(element, !sensitivity);
	}
}

void
beasy_toggle_showhide(GtkWidget *widget, GtkWidget *to_toggle)
{
	if (to_toggle == NULL)
		return;

	if (GTK_WIDGET_VISIBLE(to_toggle))
		gtk_widget_hide(to_toggle);
	else
		gtk_widget_show(to_toggle);
}

GtkWidget *beasy_separator(GtkWidget *menu)
{
	GtkWidget *menuitem;

	menuitem = gtk_separator_menu_item_new();
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	return menuitem;
}

GtkWidget *beasy_new_item(GtkWidget *menu, const char *str)
{
	GtkWidget *menuitem;
	GtkWidget *label;

	menuitem = gtk_menu_item_new();
	if (menu)
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show(menuitem);

	label = gtk_label_new(str);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_label_set_pattern(GTK_LABEL(label), "_");
	gtk_container_add(GTK_CONTAINER(menuitem), label);
	gtk_widget_show(label);
/* FIXME: Go back and fix this
	gtk_widget_add_accelerator(menuitem, "activate", accel, str[0],
				   GDK_MOD1_MASK, GTK_ACCEL_LOCKED);
*/
	beasy_set_accessible_label (menuitem, label);
	return menuitem;
}

GtkWidget *beasy_new_check_item(GtkWidget *menu, const char *str,
		GtkSignalFunc sf, gpointer data, gboolean checked)
{
	GtkWidget *menuitem;
	menuitem = gtk_check_menu_item_new_with_mnemonic(str);

	if (menu)
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), checked);

	if (sf)
		g_signal_connect(G_OBJECT(menuitem), "activate", sf, data);

	gtk_widget_show_all(menuitem);

	return menuitem;
}

GtkWidget *
beasy_pixbuf_toolbar_button_from_stock(const char *icon)
{
	GtkWidget *button, *image, *bbox;

	button = gtk_toggle_button_new();
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

	bbox = gtk_vbox_new(FALSE, 0);

	gtk_container_add (GTK_CONTAINER(button), bbox);

	image = gtk_image_new_from_stock(icon, gtk_icon_size_from_name(BEASY_ICON_SIZE_TANGO_EXTRA_SMALL));
	gtk_box_pack_start(GTK_BOX(bbox), image, FALSE, FALSE, 0);

	gtk_widget_show_all(bbox);

	return button;
}

GtkWidget *
beasy_pixbuf_button_from_stock(const char *text, const char *icon,
							  BeasyButtonOrientation style)
{
	GtkWidget *button, *image, *label, *bbox, *ibox, *lbox = NULL;

	button = gtk_button_new();

	if (style == BEASY_BUTTON_HORIZONTAL) {
		bbox = gtk_hbox_new(FALSE, 0);
		ibox = gtk_hbox_new(FALSE, 0);
		if (text)
			lbox = gtk_hbox_new(FALSE, 0);
	} else {
		bbox = gtk_vbox_new(FALSE, 0);
		ibox = gtk_vbox_new(FALSE, 0);
		if (text)
			lbox = gtk_vbox_new(FALSE, 0);
	}

	gtk_container_add(GTK_CONTAINER(button), bbox);

	if (icon) {
		gtk_box_pack_start_defaults(GTK_BOX(bbox), ibox);
		image = gtk_image_new_from_stock(icon, GTK_ICON_SIZE_BUTTON);
		gtk_box_pack_end(GTK_BOX(ibox), image, FALSE, TRUE, 0);
	}

	if (text) {
		gtk_box_pack_start_defaults(GTK_BOX(bbox), lbox);
		label = gtk_label_new(NULL);
		gtk_label_set_text_with_mnemonic(GTK_LABEL(label), text);
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), button);
		gtk_box_pack_start(GTK_BOX(lbox), label, FALSE, TRUE, 0);
		beasy_set_accessible_label (button, label);
	}

	gtk_widget_show_all(bbox);

	return button;
}


GtkWidget *beasy_new_item_from_stock(GtkWidget *menu, const char *str, const char *icon, GtkSignalFunc sf, gpointer data, guint accel_key, guint accel_mods, char *mod)
{
	GtkWidget *menuitem;
	/*
	GtkWidget *hbox;
	GtkWidget *label;
	*/
	GtkWidget *image;

	if (icon == NULL)
		menuitem = gtk_menu_item_new_with_mnemonic(str);
	else
		menuitem = gtk_image_menu_item_new_with_mnemonic(str);

	if (menu)
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	if (sf)
		g_signal_connect(G_OBJECT(menuitem), "activate", sf, data);

	if (icon != NULL) {
		image = gtk_image_new_from_stock(icon, gtk_icon_size_from_name(BEASY_ICON_SIZE_TANGO_EXTRA_SMALL));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	}
/* FIXME: this isn't right
	if (mod) {
		label = gtk_label_new(mod);
		gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 2);
		gtk_widget_show(label);
	}
*/
/*
	if (accel_key) {
		gtk_widget_add_accelerator(menuitem, "activate", accel, accel_key,
					   accel_mods, GTK_ACCEL_LOCKED);
	}
*/

	gtk_widget_show_all(menuitem);

	return menuitem;
}

GtkWidget *
beasy_make_frame(GtkWidget *parent, const char *title)
{
	GtkWidget *vbox, *label, *hbox;
	char *labeltitle;

	vbox = gtk_vbox_new(FALSE, BEASY_HIG_BOX_SPACE);
	gtk_box_pack_start(GTK_BOX(parent), vbox, FALSE, FALSE, 0);
	gtk_widget_show(vbox);

	label = gtk_label_new(NULL);

	labeltitle = g_strdup_printf("<span weight=\"bold\">%s</span>", title);
	gtk_label_set_markup(GTK_LABEL(label), labeltitle);
	g_free(labeltitle);

	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	beasy_set_accessible_label (vbox, label);

	hbox = gtk_hbox_new(FALSE, BEASY_HIG_BOX_SPACE);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gtk_label_new("    ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	vbox = gtk_vbox_new(FALSE, BEASY_HIG_BOX_SPACE);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show(vbox);

	return vbox;
}

static gpointer
aop_option_menu_get_selected(GtkWidget *optmenu, GtkWidget **p_item)
{
	GtkWidget *menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(optmenu));
	GtkWidget *item = gtk_menu_get_active(GTK_MENU(menu));
	if (p_item)
		(*p_item) = item;
	return g_object_get_data(G_OBJECT(item), "aop_per_item_data");
}

static void
aop_menu_cb(GtkWidget *optmenu, GCallback cb)
{
	GtkWidget *item;
	gpointer per_item_data;

	per_item_data = aop_option_menu_get_selected(optmenu, &item);

	if (cb != NULL) {
		((void (*)(GtkWidget *, gpointer, gpointer))cb)(item, per_item_data, g_object_get_data(G_OBJECT(optmenu), "user_data"));
	}
}

static GtkWidget *
aop_menu_item_new(GtkSizeGroup *sg, GdkPixbuf *pixbuf, const char *lbl, gpointer per_item_data, const char *data)
{
	GtkWidget *item;
	GtkWidget *hbox;
	GtkWidget *image;
	GtkWidget *label;

	item = gtk_menu_item_new();
	gtk_widget_show(item);

	hbox = gtk_hbox_new(FALSE, 4);
	gtk_widget_show(hbox);

	/* Create the image */
	if (pixbuf == NULL)
		image = gtk_image_new();
	else
		image = gtk_image_new_from_pixbuf(pixbuf);
	gtk_widget_show(image);

	if (sg)
		gtk_size_group_add_widget(sg, image);

	/* Create the label */
	label = gtk_label_new (lbl);
	gtk_widget_show (label);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	
	gtk_container_add(GTK_CONTAINER(item), hbox);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

	g_object_set_data(G_OBJECT (item), data, per_item_data);
	g_object_set_data(G_OBJECT (item), "aop_per_item_data", per_item_data);

	beasy_set_accessible_label(item, label);

	return item;
}

static GtkWidget *
aop_option_menu_new(AopMenu *aop_menu, GCallback cb, gpointer user_data)
{
	GtkWidget *optmenu;

	optmenu = gtk_option_menu_new();
	gtk_widget_show(optmenu);
	gtk_option_menu_set_menu(GTK_OPTION_MENU(optmenu), aop_menu->menu);

	if (aop_menu->default_item != -1)
		gtk_option_menu_set_history(GTK_OPTION_MENU(optmenu), aop_menu->default_item);

	g_object_set_data_full(G_OBJECT(optmenu), "aop_menu", aop_menu, (GDestroyNotify)g_free);
	g_object_set_data(G_OBJECT(optmenu), "user_data", user_data);

	g_signal_connect(G_OBJECT(optmenu), "changed", G_CALLBACK(aop_menu_cb), cb);

	return optmenu;
}

static void
aop_option_menu_replace_menu(GtkWidget *optmenu, AopMenu *new_aop_menu)
{
	if (gtk_option_menu_get_menu(GTK_OPTION_MENU(optmenu)))
		gtk_option_menu_remove_menu(GTK_OPTION_MENU(optmenu));

	gtk_option_menu_set_menu(GTK_OPTION_MENU(optmenu), new_aop_menu->menu);

	if (new_aop_menu->default_item != -1)
		gtk_option_menu_set_history(GTK_OPTION_MENU(optmenu), new_aop_menu->default_item);

	g_object_set_data_full(G_OBJECT(optmenu), "aop_menu", new_aop_menu, (GDestroyNotify)g_free);
}

static void
aop_option_menu_select_by_data(GtkWidget *optmenu, gpointer data)
{
	guint idx;
	GList *llItr = NULL;

	for (idx = 0, llItr = GTK_MENU_SHELL(gtk_option_menu_get_menu(GTK_OPTION_MENU(optmenu)))->children;
	     llItr != NULL;
	     llItr = llItr->next, idx++) {
		if (data == g_object_get_data(G_OBJECT(llItr->data), "aop_per_item_data")) {
			gtk_option_menu_set_history(GTK_OPTION_MENU(optmenu), idx);
			break;
		}
	}
}

gboolean
beasy_check_if_dir(const char *path, GtkFileSelection *filesel)
{
	char *dirname = NULL;

	if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
		/* append a / if needed */
		if (path[strlen(path) - 1] != G_DIR_SEPARATOR) {
			dirname = g_strconcat(path, G_DIR_SEPARATOR_S, NULL);
		}
		gtk_file_selection_set_filename(filesel, (dirname != NULL) ? dirname : path);
		g_free(dirname);
		return TRUE;
	}

	return FALSE;
}

void
beasy_setup_gtkspell(GtkTextView *textview)
{
#ifdef USE_GTKSPELL
	GError *error = NULL;
	char *locale = NULL;

	g_return_if_fail(textview != NULL);
	g_return_if_fail(GTK_IS_TEXT_VIEW(textview));

	if (gtkspell_new_attach(textview, locale, &error) == NULL && error)
	{
		Oul_debug_warning("gtkspell", "Failed to setup GtkSpell: %s\n",
						   error->message);
		g_error_free(error);
	}
#endif /* USE_GTKSPELL */
}

void
beasy_save_accels_cb(GtkAccelGroup *accel_group, guint arg1,
                         GdkModifierType arg2, GClosure *arg3,
                         gpointer data)
{
	oul_debug_misc("accels", "accel changed, scheduling save.\n");

	if (!accels_save_timer)
		accels_save_timer = g_timeout_add(5000, beasy_save_accels,
		                                  NULL);
}

gboolean
beasy_save_accels(gpointer data)
{
	char *filename = NULL;

	filename = g_build_filename(oul_user_dir(), G_DIR_SEPARATOR_S, "accels", NULL);
	oul_debug_misc("accels", "saving accels to %s\n", filename);
	gtk_accel_map_save(filename);
	g_free(filename);

	accels_save_timer = 0;
	return FALSE;
}

void
beasy_load_accels()
{
	char *filename = NULL;

	filename = g_build_filename(oul_user_dir(), G_DIR_SEPARATOR_S, "accels", NULL);
	gtk_accel_map_load(filename);
	g_free(filename);
}

void
beasy_set_accessible_label (GtkWidget *w, GtkWidget *l)
{
	AtkObject *acc;
	const gchar *label_text;
	const gchar *existing_name;

	acc = gtk_widget_get_accessible (w);

	/* If this object has no name, set it's name with the label text */
	existing_name = atk_object_get_name (acc);
	if (!existing_name) {
		label_text = gtk_label_get_text (GTK_LABEL(l));
		if (label_text)
			atk_object_set_name (acc, label_text);
	}

	beasy_set_accessible_relations(w, l);
}

void
beasy_set_accessible_relations (GtkWidget *w, GtkWidget *l)
{
	AtkObject *acc, *label;
	AtkObject *rel_obj[1];
	AtkRelationSet *set;
	AtkRelation *relation;

	acc = gtk_widget_get_accessible (w);
	label = gtk_widget_get_accessible (l);

	/* Make sure mnemonics work */
	gtk_label_set_mnemonic_widget(GTK_LABEL(l), w);

	/* Create the labeled-by relation */
	set = atk_object_ref_relation_set (acc);
	rel_obj[0] = label;
	relation = atk_relation_new (rel_obj, 1, ATK_RELATION_LABELLED_BY);
	atk_relation_set_add (set, relation);
	g_object_unref (relation);
	g_object_unref(set);

	/* Create the label-for relation */
	set = atk_object_ref_relation_set (label);
	rel_obj[0] = acc;
	relation = atk_relation_new (rel_obj, 1, ATK_RELATION_LABEL_FOR);
	atk_relation_set_add (set, relation);
	g_object_unref (relation);
	g_object_unref(set);
}

void
beasy_menu_position_func_helper(GtkMenu *menu,
							gint *x,
							gint *y,
							gboolean *push_in,
							gpointer data)
{
#if GTK_CHECK_VERSION(2,2,0)
	GtkWidget *widget;
	GtkRequisition requisition;
	GdkScreen *screen;
	GdkRectangle monitor;
	gint monitor_num;
	gint space_left, space_right, space_above, space_below;
	gint needed_width;
	gint needed_height;
	gint xthickness;
	gint ythickness;
	gboolean rtl;

	g_return_if_fail(GTK_IS_MENU(menu));

	widget     = GTK_WIDGET(menu);
	screen     = gtk_widget_get_screen(widget);
	xthickness = widget->style->xthickness;
	ythickness = widget->style->ythickness;
	rtl        = (gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL);

	/*
	 * We need the requisition to figure out the right place to
	 * popup the menu. In fact, we always need to ask here, since
	 * if a size_request was queued while we weren't popped up,
	 * the requisition won't have been recomputed yet.
	 */
	gtk_widget_size_request (widget, &requisition);

	monitor_num = gdk_screen_get_monitor_at_point (screen, *x, *y);

	push_in = FALSE;

	/*
	 * The placement of popup menus horizontally works like this (with
	 * RTL in parentheses)
	 *
	 * - If there is enough room to the right (left) of the mouse cursor,
	 *   position the menu there.
	 *
	 * - Otherwise, if if there is enough room to the left (right) of the
	 *   mouse cursor, position the menu there.
	 *
	 * - Otherwise if the menu is smaller than the monitor, position it
	 *   on the side of the mouse cursor that has the most space available
	 *
	 * - Otherwise (if there is simply not enough room for the menu on the
	 *   monitor), position it as far left (right) as possible.
	 *
	 * Positioning in the vertical direction is similar: first try below
	 * mouse cursor, then above.
	 */
	gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

	space_left = *x - monitor.x;
	space_right = monitor.x + monitor.width - *x - 1;
	space_above = *y - monitor.y;
	space_below = monitor.y + monitor.height - *y - 1;

	/* position horizontally */

	/* the amount of space we need to position the menu. Note the
	 * menu is offset "xthickness" pixels
	 */
	needed_width = requisition.width - xthickness;

	if (needed_width <= space_left ||
	    needed_width <= space_right)
	{
		if ((rtl  && needed_width <= space_left) ||
		    (!rtl && needed_width >  space_right))
		{
			/* position left */
			*x = *x + xthickness - requisition.width + 1;
		}
		else
		{
			/* position right */
			*x = *x - xthickness;
		}

		/* x is clamped on-screen further down */
	}
	else if (requisition.width <= monitor.width)
	{
		/* the menu is too big to fit on either side of the mouse
		 * cursor, but smaller than the monitor. Position it on
		 * the side that has the most space
		 */
		if (space_left > space_right)
		{
			/* left justify */
			*x = monitor.x;
		}
		else
		{
			/* right justify */
			*x = monitor.x + monitor.width - requisition.width;
		}
	}
	else /* menu is simply too big for the monitor */
	{
		if (rtl)
		{
			/* right justify */
			*x = monitor.x + monitor.width - requisition.width;
		}
		else
		{
			/* left justify */
			*x = monitor.x;
		}
	}

	/* Position vertically. The algorithm is the same as above, but
	 * simpler because we don't have to take RTL into account.
	 */
	needed_height = requisition.height - ythickness;

	if (needed_height <= space_above ||
	    needed_height <= space_below)
	{
		if (needed_height <= space_below)
			*y = *y - ythickness;
		else
			*y = *y + ythickness - requisition.height + 1;

		*y = CLAMP (*y, monitor.y,
			   monitor.y + monitor.height - requisition.height);
	}
	else if (needed_height > space_below && needed_height > space_above)
	{
		if (space_below >= space_above)
			*y = monitor.y + monitor.height - requisition.height;
		else
			*y = monitor.y;
	}
	else
	{
		*y = monitor.y;
	}
#endif
}


void
beasy_treeview_popup_menu_position_func(GtkMenu *menu,
										   gint *x,
										   gint *y,
										   gboolean *push_in,
										   gpointer data)
{
	GtkWidget *widget = GTK_WIDGET(data);
	GtkTreeView *tv = GTK_TREE_VIEW(data);
	GtkTreePath *path;
	GtkTreeViewColumn *col;
	GdkRectangle rect;
	gint ythickness = GTK_WIDGET(menu)->style->ythickness;

	gdk_window_get_origin (widget->window, x, y);
	gtk_tree_view_get_cursor (tv, &path, &col);
	gtk_tree_view_get_cell_area (tv, path, col, &rect);

	*x += rect.x+rect.width;
	*y += rect.y+rect.height+ythickness;
	beasy_menu_position_func_helper(menu, x, y, push_in, data);
}

GdkPixbuf *
beasy_create_status_icon(OulStatusPrimitive prim, GtkWidget *w, const char *size)
{
	GtkIconSize icon_size = gtk_icon_size_from_name(size);
	GdkPixbuf *pixbuf = NULL;

	if (prim == OUL_STATUS_OFFLINE)
		pixbuf = gtk_widget_render_icon (w, BEASY_STOCK_STATUS_OFFLINE,
				icon_size, "GtkWidget");
	else
		pixbuf = gtk_widget_render_icon (w, BEASY_STOCK_STATUS_AVAILABLE,
				icon_size, "GtkWidget");
	return pixbuf;

}

static void
menu_action_cb(GtkMenuItem *item, gpointer object)
{
	gpointer data;
	void (*callback)(gpointer, gpointer);

	callback = g_object_get_data(G_OBJECT(item), "Oulcallback");
	data = g_object_get_data(G_OBJECT(item), "Oulcallbackdata");

	if (callback)
		callback(object, data);
}

GtkWidget *
beasy_append_menu_action(GtkWidget *menu, OulMenuAction *act,
                            gpointer object)
{
	GtkWidget *menuitem;

	if (act == NULL) {
		return beasy_separator(menu);
	}

	if (act->children == NULL) {
		menuitem = gtk_menu_item_new_with_mnemonic(act->label);

		if (act->callback != NULL) {
			g_object_set_data(G_OBJECT(menuitem),
							  "oulcallback",
							  act->callback);
			g_object_set_data(G_OBJECT(menuitem),
							  "oulcallbackdata",
							  act->data);
			g_signal_connect(G_OBJECT(menuitem), "activate",
							 G_CALLBACK(menu_action_cb),
							 object);
		} else {
			gtk_widget_set_sensitive(menuitem, FALSE);
		}

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	} else {
		GList *l = NULL;
		GtkWidget *submenu = NULL;
		GtkAccelGroup *group;

		menuitem = gtk_menu_item_new_with_mnemonic(act->label);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

		submenu = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), submenu);

		group = gtk_menu_get_accel_group(GTK_MENU(menu));
		if (group) {
			char *path = g_strdup_printf("%s/%s", GTK_MENU_ITEM(menuitem)->accel_path, act->label);
			gtk_menu_set_accel_path(GTK_MENU(submenu), path);
			g_free(path);
			gtk_menu_set_accel_group(GTK_MENU(submenu), group);
		}

		for (l = act->children; l; l = l->next) {
			OulMenuAction *act = (OulMenuAction *)l->data;

			beasy_append_menu_action(submenu, act, object);
		}
		g_list_free(act->children);
		act->children = NULL;
	}
	oul_menu_action_free(act);
	return menuitem;
}

void beasy_set_cursor(GtkWidget *widget, GdkCursorType cursor_type)
{
	GdkCursor *cursor;

	g_return_if_fail(widget != NULL);
	if (widget->window == NULL)
		return;

	cursor = gdk_cursor_new(cursor_type);
	gdk_window_set_cursor(widget->window, cursor);
	gdk_cursor_unref(cursor);

#if GTK_CHECK_VERSION(2,4,0)
	gdk_display_flush(gdk_drawable_get_display(GDK_DRAWABLE(widget->window)));
#else
	gdk_flush();
#endif
}

void beasy_clear_cursor(GtkWidget *widget)
{
	g_return_if_fail(widget != NULL);
	if (widget->window == NULL)
		return;

	gdk_window_set_cursor(widget->window, NULL);
}

#if GTK_CHECK_VERSION(2,2,0)
static gboolean
str_array_match(char **a, char **b)
{
	int i, j;

	if (!a || !b)
		return FALSE;
	for (i = 0; a[i] != NULL; i++)
		for (j = 0; b[j] != NULL; j++)
			if (!g_ascii_strcasecmp(a[i], b[j]))
				return TRUE;
	return FALSE;
}
#endif

#if !GTK_CHECK_VERSION(2,6,0)
static void
_gdk_file_scale_size_prepared_cb (GdkPixbufLoader *loader,
		  int              width,
		  int              height,
		  gpointer         data)
{
	struct {
		gint width;
		gint height;
		gboolean preserve_aspect_ratio;
	} *info = data;

	g_return_if_fail (width > 0 && height > 0);

	if (info->preserve_aspect_ratio &&
		(info->width > 0 || info->height > 0)) {
		if (info->width < 0)
		{
			width = width * (double)info->height/(double)height;
			height = info->height;
		}
		else if (info->height < 0)
		{
			height = height * (double)info->width/(double)width;
			width = info->width;
		}
		else if ((double)height * (double)info->width >
				 (double)width * (double)info->height) {
			width = 0.5 + (double)width * (double)info->height / (double)height;
			height = info->height;
		} else {
			height = 0.5 + (double)height * (double)info->width / (double)width;
			width = info->width;
		}
	} else {
			if (info->width > 0)
				width = info->width;
			if (info->height > 0)
				height = info->height;
	}

#if GTK_CHECK_VERSION(2,2,0) /* 2.0 users are going to have very strangely sized things */
	gdk_pixbuf_loader_set_size (loader, width, height);
#else
#warning  nosnilmot could not be bothered to fix this properly for you
#warning  ... good luck ... your images may end up strange sizes
#endif
}

GdkPixbuf *
gdk_pixbuf_new_from_file_at_scale(const char *filename, int width, int height,
				  				  gboolean preserve_aspect_ratio,
								  GError **error)
{
	GdkPixbufLoader *loader;
	GdkPixbuf       *pixbuf;
	guchar buffer [4096];
	int length;
	FILE *f;
	struct {
		gint width;
		gint height;
		gboolean preserve_aspect_ratio;
	} info;
	GdkPixbufAnimation *animation;
	GdkPixbufAnimationIter *iter;
	gboolean has_frame;

	g_return_val_if_fail (filename != NULL, NULL);
	g_return_val_if_fail (width > 0 || width == -1, NULL);
	g_return_val_if_fail (height > 0 || height == -1, NULL);

	f = g_fopen (filename, "rb");
	if (!f) {
		gint save_errno = errno;
		gchar *display_name = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
		g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (save_errno),
					 _("Failed to open file '%s': %s"),
					 display_name ? display_name : "(unknown)",
					 g_strerror (save_errno));
		g_free (display_name);
		return NULL;
	}

	loader = gdk_pixbuf_loader_new ();

	info.width = width;
	info.height = height;
	info.preserve_aspect_ratio = preserve_aspect_ratio;

	g_signal_connect (loader, "size-prepared", G_CALLBACK (_gdk_file_scale_size_prepared_cb), &info);

	has_frame = FALSE;
	while (!has_frame && !feof (f) && !ferror (f)) {
		length = fread (buffer, 1, sizeof (buffer), f);
		if (length > 0)
			if (!gdk_pixbuf_loader_write (loader, buffer, length, error)) {
				gdk_pixbuf_loader_close (loader, NULL);
				fclose (f);
				g_object_unref (loader);
				return NULL;
			}

		animation = gdk_pixbuf_loader_get_animation (loader);
		if (animation) {
			iter = gdk_pixbuf_animation_get_iter (animation, 0);
			if (!gdk_pixbuf_animation_iter_on_currently_loading_frame (iter)) {
				has_frame = TRUE;
			}
			g_object_unref (iter);
		}
	}

	fclose (f);

	if (!gdk_pixbuf_loader_close (loader, error) && !has_frame) {
		g_object_unref (loader);
		return NULL;
	}

	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

	if (!pixbuf) {
		gchar *display_name = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
		g_object_unref (loader);
		g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_FAILED,
					 _("Failed to load image '%s': reason not known, probably a corrupt image file"),
					 display_name ? display_name : "(unknown)");
		g_free (display_name);
		return NULL;
	}

	g_object_ref (pixbuf);

	g_object_unref (loader);

	return pixbuf;
}
#endif /* ! Gtk 2.6.0 */

char *beasy_make_pretty_arrows(const char *str)
{
	char *ret;
	char **split = g_strsplit(str, "->", -1);
	ret = g_strjoinv("\342\207\250", split);
	g_strfreev(split);

	split = g_strsplit(ret, "<-", -1);
	g_free(ret);
	ret = g_strjoinv("\342\207\246", split);
	g_strfreev(split);

	return ret;
}

void beasy_set_urgent(GtkWindow *window, gboolean urgent)
{
#if GTK_CHECK_VERSION(2,8,0)
	gtk_window_set_urgency_hint(window, urgent);
#elif defined GDK_WINDOWING_X11
	GdkWindow *gdkwin;
	XWMHints *hints;

	g_return_if_fail(window != NULL);

	gdkwin = GTK_WIDGET(window)->window;

	g_return_if_fail(gdkwin != NULL);

	hints = XGetWMHints(GDK_WINDOW_XDISPLAY(gdkwin),
	                    GDK_WINDOW_XWINDOW(gdkwin));
	if(!hints)
		hints = XAllocWMHints();

	if (urgent)
		hints->flags |= XUrgencyHint;
	else
		hints->flags &= ~XUrgencyHint;
	XSetWMHints(GDK_WINDOW_XDISPLAY(gdkwin),
	            GDK_WINDOW_XWINDOW(gdkwin), hints);
	XFree(hints);
#else
	/* do something else? */
#endif
}

GSList *minidialogs = NULL;

static void *
beasy_utils_get_handle(void)
{
	static int handle;

	return &handle;
}

static void
alert_killed_cb(GtkWidget *widget)
{
	minidialogs = g_slist_remove(minidialogs, widget);
}

struct _old_button_clicked_cb_data
{
	BeasyUtilMiniDialogCallback cb;
	gpointer data;
};

static void
old_mini_dialog_destroy_cb(GtkWidget *dialog,
                           GList *cb_datas)
{
	while (cb_datas != NULL)
	{
		g_free(cb_datas->data);
		cb_datas = g_list_delete_link(cb_datas, cb_datas);
	}
}

/*
 * "This is so dead sexy."
 * "Two thumbs up."
 * "Best movie of the year."
 *
 * This is the function that handles CTRL+F searching in the buddy list.
 * It finds the top-most buddy/group/chat/whatever containing the
 * entered string.
 *
 * It's somewhat ineffecient, because we strip all the HTML from the
 * "name" column of the buddy list (because the GtkTreeModel does not
 * contain the screen name in a non-markedup format).  But the alternative
 * is to add an extra column to the GtkTreeModel.  And this function is
 * used rarely, so it shouldn't matter TOO much.
 */
gboolean beasy_tree_view_search_equal_func(GtkTreeModel *model, gint column,
			const gchar *key, GtkTreeIter *iter, gpointer data)
{
	gchar *enteredstring;
	gchar *tmp;
	gchar *withmarkup;
	gchar *nomarkup;
	gchar *normalized;
	gboolean result;
	size_t i;
	size_t len;
	PangoLogAttr *log_attrs;
	gchar *word;

	if (g_ascii_strcasecmp(key, "Global Thermonuclear War") == 0)
	{
		oul_notify_info(NULL, "WOPR",
				"Wouldn't you prefer a nice game of chess?", NULL);
		return FALSE;
	}

	gtk_tree_model_get(model, iter, column, &withmarkup, -1);
	if (withmarkup == NULL)   /* This is probably a separator */
		return TRUE;

	tmp = g_utf8_normalize(key, -1, G_NORMALIZE_DEFAULT);
	enteredstring = g_utf8_casefold(tmp, -1);
	g_free(tmp);

	nomarkup = oul_markup_strip_html(withmarkup);
	tmp = g_utf8_normalize(nomarkup, -1, G_NORMALIZE_DEFAULT);
	g_free(nomarkup);
	normalized = g_utf8_casefold(tmp, -1);
	g_free(tmp);

	if (oul_str_has_prefix(normalized, enteredstring))
	{
		g_free(withmarkup);
		g_free(enteredstring);
		g_free(normalized);
		return FALSE;
	}


	/* Use Pango to separate by words. */
	len = g_utf8_strlen(normalized, -1);
	log_attrs = g_new(PangoLogAttr, len + 1);

	pango_get_log_attrs(normalized, strlen(normalized), -1, NULL, log_attrs, len + 1);

	word = normalized;
	result = TRUE;
	for (i = 0; i < (len - 1) ; i++)
	{
		if (log_attrs[i].is_word_start &&
		    oul_str_has_prefix(word, enteredstring))
		{
			result = FALSE;
			break;
		}
		word = g_utf8_next_char(word);
	}
	g_free(log_attrs);

/* The non-Pango version. */
#if 0
	word = normalized;
	result = TRUE;
	while (word[0] != '\0')
	{
		gunichar c = g_utf8_get_char(word);
		if (!g_unichar_isalnum(c))
		{
			word = g_utf8_find_next_char(word, NULL);
			if (Oul_str_has_prefix(word, enteredstring))
			{
				result = FALSE;
				break;
			}
		}
		else
			word = g_utf8_find_next_char(word, NULL);
	}
#endif

	g_free(withmarkup);
	g_free(enteredstring);
	g_free(normalized);

	return result;
}


gboolean beasy_gdk_pixbuf_is_opaque(GdkPixbuf *pixbuf) {
        int width, height, rowstride, i;
        unsigned char *pixels;
        unsigned char *row;

        if (!gdk_pixbuf_get_has_alpha(pixbuf))
                return TRUE;

        width = gdk_pixbuf_get_width (pixbuf);
        height = gdk_pixbuf_get_height (pixbuf);
        rowstride = gdk_pixbuf_get_rowstride (pixbuf);
        pixels = gdk_pixbuf_get_pixels (pixbuf);

        row = pixels;
        for (i = 3; i < rowstride; i+=4) {
                if (row[i] < 0xfe)
                        return FALSE;
        }

        for (i = 1; i < height - 1; i++) {
                row = pixels + (i*rowstride);
                if (row[3] < 0xfe || row[rowstride-1] < 0xfe) {
                        return FALSE;
            }
        }

        row = pixels + ((height-1) * rowstride);
        for (i = 3; i < rowstride; i+=4) {
                if (row[i] < 0xfe)
                        return FALSE;
        }

        return TRUE;
}

void beasy_gdk_pixbuf_make_round(GdkPixbuf *pixbuf) {
	int width, height, rowstride;
        guchar *pixels;
        if (!gdk_pixbuf_get_has_alpha(pixbuf))
                return;
        width = gdk_pixbuf_get_width(pixbuf);
        height = gdk_pixbuf_get_height(pixbuf);
        rowstride = gdk_pixbuf_get_rowstride(pixbuf);
        pixels = gdk_pixbuf_get_pixels(pixbuf);

        if (width < 6 || height < 6)
                return;
        /* Top left */
        pixels[3] = 0;
        pixels[7] = 0x80;
        pixels[11] = 0xC0;
        pixels[rowstride + 3] = 0x80;
        pixels[rowstride * 2 + 3] = 0xC0;

        /* Top right */
        pixels[width * 4 - 1] = 0;
        pixels[width * 4 - 5] = 0x80;
        pixels[width * 4 - 9] = 0xC0;
        pixels[rowstride + (width * 4) - 1] = 0x80;
        pixels[(2 * rowstride) + (width * 4) - 1] = 0xC0;

        /* Bottom left */
        pixels[(height - 1) * rowstride + 3] = 0;
        pixels[(height - 1) * rowstride + 7] = 0x80;
        pixels[(height - 1) * rowstride + 11] = 0xC0;
        pixels[(height - 2) * rowstride + 3] = 0x80;
        pixels[(height - 3) * rowstride + 3] = 0xC0;

        /* Bottom right */
        pixels[height * rowstride - 1] = 0;
        pixels[(height - 1) * rowstride - 1] = 0x80;
        pixels[(height - 2) * rowstride - 1] = 0xC0;
        pixels[height * rowstride - 5] = 0x80;
        pixels[height * rowstride - 9] = 0xC0;
}

const char *beasy_get_dim_grey_string(GtkWidget *widget) {
	static char dim_grey_string[8] = "";
	GtkStyle *style;

	if (!widget)
		return "dim grey";

 	style = gtk_widget_get_style(widget);
	if (!style)
		return "dim grey";
	
	snprintf(dim_grey_string, sizeof(dim_grey_string), "#%02x%02x%02x",
	style->text_aa[GTK_STATE_NORMAL].red >> 8,
	style->text_aa[GTK_STATE_NORMAL].green >> 8,
	style->text_aa[GTK_STATE_NORMAL].blue >> 8);
	return dim_grey_string;
}

#if !GTK_CHECK_VERSION(2,2,0)
GtkTreePath *
gtk_tree_path_new_from_indices (gint first_index, ...)
{
	int arg;
	va_list args;
	GtkTreePath *path;

	path = gtk_tree_path_new ();

	va_start (args, first_index);
	arg = first_index;

	while (arg != -1)
	{
		gtk_tree_path_append_index (path, arg);
		arg = va_arg (args, gint);
	}

	va_end (args);

	return path;
}
#endif

static void
combo_box_changed_cb(GtkComboBox *combo_box, GtkEntry *entry)
{
#if GTK_CHECK_VERSION(2, 6, 0)
	char *text = gtk_combo_box_get_active_text(combo_box);
#else
	GtkWidget *widget = gtk_bin_get_child(GTK_BIN(combo_box));
	char *text = g_strdup(gtk_entry_get_text( GTK_ENTRY(widget)));
#endif
	
	gtk_entry_set_text(entry, text ? text : "");
	g_free(text);
}

static gboolean
entry_key_pressed_cb(GtkWidget *entry, GdkEventKey *key, GtkComboBox *combo)
{
	if (key->keyval == GDK_Down || key->keyval == GDK_Up) {
		gtk_combo_box_popup(combo);
		return TRUE;
	}
	return FALSE;
}

GtkWidget *
beasy_text_combo_box_entry_new(const char *default_item, GList *items)
{
	GtkComboBox *ret = NULL;
	GtkWidget *the_entry = NULL;

	ret = GTK_COMBO_BOX(gtk_combo_box_new_text());
	the_entry = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(ret), the_entry);

	if (default_item)
		gtk_entry_set_text(GTK_ENTRY(the_entry), default_item);

	for (; items != NULL ; items = items->next) {
		char *text = items->data;
		if (text && *text)
			gtk_combo_box_append_text(ret, text);
	}

	g_signal_connect(G_OBJECT(ret), "changed", (GCallback)combo_box_changed_cb, the_entry);
	g_signal_connect_after(G_OBJECT(the_entry), "key-press-event", G_CALLBACK(entry_key_pressed_cb), ret);

	return GTK_WIDGET(ret);
}

const char *beasy_text_combo_box_entry_get_text(GtkWidget *widget)
{
	return gtk_entry_get_text(GTK_ENTRY(GTK_BIN((widget))->child));
}

void beasy_text_combo_box_entry_set_text(GtkWidget *widget, const char *text)
{
	gtk_entry_set_text(GTK_ENTRY(GTK_BIN((widget))->child), (text));
}

GtkWidget *
beasy_add_widget_to_vbox(GtkBox *vbox, const char *widget_label, GtkSizeGroup *sg, GtkWidget *widget, gboolean expand, GtkWidget **p_label)
{
	GtkWidget *hbox;
	GtkWidget *label = NULL;

	if (widget_label) {
		hbox = gtk_hbox_new(FALSE, 5);
		gtk_widget_show(hbox);
		gtk_box_pack_start(vbox, hbox, FALSE, FALSE, 0);

		label = gtk_label_new_with_mnemonic(widget_label);
		gtk_widget_show(label);
		if (sg) {
			gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
			gtk_size_group_add_widget(sg, label);
		}
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	} else {
		hbox = GTK_WIDGET(vbox);
	}

	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, expand, TRUE, 0);
	if (label) {
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), widget);
		beasy_set_accessible_label (widget, label);
	}

	if (p_label)
		(*p_label) = label;
	return hbox;
}

gboolean beasy_auto_parent_window(GtkWidget *widget)
{
#if GTK_CHECK_VERSION(2,4,0)
	/* This finds the currently active window and makes that the parent window. */
	GList *windows = NULL;
	GtkWidget *parent = NULL;
	GdkEvent *event = gtk_get_current_event();
	GdkWindow *menu = NULL;

	if (event == NULL)
		/* The window was not triggered by a user action. */
		return FALSE;

	/* We need to special case events from a popup menu. */
	if (event->type == GDK_BUTTON_RELEASE) {
		/* XXX: Neither of the following works:
			menu = event->button.window;
			menu = gdk_window_get_parent(event->button.window);
			menu = gdk_window_get_toplevel(event->button.window);
		*/
	} else if (event->type == GDK_KEY_PRESS)
		menu = event->key.window;

	windows = gtk_window_list_toplevels();
	while (windows) {
		GtkWidget *window = windows->data;
		windows = g_list_delete_link(windows, windows);

		if (window == widget ||
				!GTK_WIDGET_VISIBLE(window)) {
			continue;
		}

		if (gtk_window_has_toplevel_focus(GTK_WINDOW(window)) ||
				(menu && menu == window->window)) {
			parent = window;
			break;
		}
	}
	if (windows)
		g_list_free(windows);
	if (parent) {
		gtk_window_set_transient_for(GTK_WINDOW(widget), GTK_WINDOW(parent));
		return TRUE;
	}
#endif

	return FALSE;
}

static GtkHtmlFuncs gtkhtml_cbs = {
	(GtkHtmlGetImageFunc)oul_imgstore_find_by_id,
	(GtkHtmlGetImageDataFunc)oul_imgstore_get_data,
	(GtkHtmlGetImageSizeFunc)oul_imgstore_get_size,
	(GtkHtmlGetImageFilenameFunc)oul_imgstore_get_filename,
	oul_imgstore_ref_by_id,
	oul_imgstore_unref_by_id,
};


void
beasy_setup_html(GtkWidget *html)
{
	PangoFontDescription *desc = NULL;
	g_return_if_fail(html != NULL);
	g_return_if_fail(GTK_IS_HTML(html));

	g_signal_connect(G_OBJECT(html), "url_clicked",
					 G_CALLBACK(url_clicked_cb), NULL);

	//beasy_themes_smiley_themeize(html);

	gtk_html_set_funcs(GTK_HTML(html), &gtkhtml_cbs);

	if (oul_running_gnome()) {
		/* Use the GNOME "document" font, if applicable */
		char *path;

		if ((path = g_find_program_in_path("gconftool-2"))) {
			char *font = NULL;
			char *err = NULL;
			g_free(path);
			if (g_spawn_command_line_sync(
					"gconftool-2 -g /desktop/gnome/interface/document_font_name",
					&font, &err, NULL, NULL)) {
				desc = pango_font_description_from_string(font);
			}
			g_free(err);
			g_free(font);
		}
	}

	if (desc) {
		gtk_widget_modify_font(html, desc);
		pango_font_description_free(desc);
	}
}


GtkWidget *
beasy_create_html(gboolean editable, GtkWidget **html_ret, GtkWidget **sw_ret)
{
	GtkWidget *frame;
	GtkWidget *html;
	GtkWidget *sep;
	GtkWidget *sw;
	GtkWidget *toolbar = NULL;
	GtkWidget *vbox;

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(vbox);

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
								   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
	gtk_widget_show(sw);

	html = gtk_html_new(NULL, NULL);
	gtk_html_set_editable(GTK_HTML(html), editable);
	gtk_html_set_format_functions(GTK_HTML(html), GTK_HTML_ALL ^ GTK_HTML_IMAGE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(html), GTK_WRAP_WORD_CHAR);
#ifdef USE_GTKSPELL
	if (editable && oul_prefs_get_bool(BEASY_PREFS_ROOT "/notification/spellcheck"))
		beasy_setup_gtkspell(GTK_TEXT_VIEW(html));
#endif
	gtk_widget_show(html);

	beasy_setup_html(html);

	gtk_container_add(GTK_CONTAINER(sw), html);

	if (html_ret != NULL)
		*html_ret = html;

	if (sw_ret != NULL)
		*sw_ret = sw;

	return frame;
}


