#ifndef _BEASY_UTILS_H_
#define _BEASY_UTILS_H_

#include "beasy.h"
#include "util.h"

typedef enum
{
	BEASY_BUTTON_HORIZONTAL,
	BEASY_BUTTON_VERTICAL

} BeasyButtonOrientation;

typedef enum
{
	BEASY_BUTTON_NONE = 0,
	BEASY_BUTTON_TEXT,
	BEASY_BUTTON_IMAGE,
	BEASY_BUTTON_TEXT_IMAGE

} BeasyButtonStyle;

typedef enum
{
	BEASY_PRPL_ICON_SMALL,
	BEASY_PRPL_ICON_MEDIUM,
	BEASY_PRPL_ICON_LARGE
} BeasyPrplIconSize;

typedef enum
{
	BEASY_BROWSER_DEFAULT = 0,
	BEASY_BROWSER_CURRENT,
	BEASY_BROWSER_NEW_WINDOW,
	BEASY_BROWSER_NEW_TAB

} BeasyBrowserPlace;

/**
 * Sets up a gtkimhtml widget, loads it with smileys, and sets the
 * default signal handlers.
 *
 * @param imhtml The gtkimhtml widget to setup.
 */
void beasy_setup_imhtml(GtkWidget *imhtml);

/**
 * Create an GtkHtml widget and associated GtkHtmlToolbar widget.  This
 * functions puts both widgets in a nice GtkFrame.  They're separate by an
 * attractive GtkSeparator.
 *
 * @param editable @c TRUE if this imhtml should be editable.  If this is @c FALSE,
 *        then the toolbar will NOT be created.  If this imthml should be
 *        read-only at first, but may become editable later, then pass in
 *        @c TRUE here and then manually call gtk_html_set_editable() later.
 * @param imhtml_ret A pointer to a pointer to a GtkWidget.  This pointer
 *        will be set to the imhtml when this function exits.
 * @param toolbar_ret A pointer to a pointer to a GtkWidget.  If editable is
 *        TRUE then this will be set to the toolbar when this function exits.
 *        Otherwise this will be set to @c NULL.
 * @param sw_ret This will be filled with a pointer to the scrolled window
 *        widget which contains the imhtml.
 * @return The GtkFrame containing the toolbar and imhtml.
 */
GtkWidget *beasy_create_html(gboolean editable, GtkWidget **imhtml_ret, GtkWidget **sw_ret);

/**
 * Creates a new window
 *
 * @param title        The window title, or @c NULL
 * @param border_width The window's desired border width
 * @param role         A string indicating what the window is responsible for doing, or @c NULL
 * @param resizable    Whether the window should be resizable (@c TRUE) or not (@c FALSE)
 *
 * @since 2.1.0
 */
GtkWidget *beasy_create_window(const char *title, guint border_width, const char *role, gboolean resizable);

/**
 * Creates a new dialog window
 *
 * @param title        The window title, or @c NULL
 * @param border_width The window's desired border width
 * @param role         A string indicating what the window is responsible for doing, or @c NULL
 * @param resizable    Whether the window should be resizable (@c TRUE) or not (@c FALSE)
 *
 * @since 2.4.0
 */
GtkWidget *beasy_create_dialog(const char *title, guint border_width, const char *role, gboolean resizable);

/**
 * Retrieves the main content box (vbox) from a beasy dialog window
 *
 * @param dialog       The dialog window
 * @param homogeneous  TRUE if all children are to be given equal space allotments. 
 * @param spacing      the number of pixels to place by default between children
 *
 * @since 2.4.0
 */
GtkWidget *beasy_dialog_get_vbox_with_properties(GtkDialog *dialog, gboolean homogeneous, gint spacing);

/**
 * Retrieves the main content box (vbox) from a beasy dialog window
 *
 * @param dialog       The dialog window
 *
 * @since 2.4.0
 */
GtkWidget *beasy_dialog_get_vbox(GtkDialog *dialog);

/**
 * Add a button to a dialog created by #beasy_create_dialog.
 *
 * @param dialog         The dialog window
 * @param label          The stock-id or the label for the button
 * @param callback       The callback function for the button
 * @param callbackdata   The user data for the callback function
 *
 * @return The created button.
 * @since 2.4.0
 */
GtkWidget *beasy_dialog_add_button(GtkDialog *dialog, const char *label,
		GCallback callback, gpointer callbackdata);

/**
 * Retrieves the action area (button box) from a beasy dialog window
 *
 * @param dialog       The dialog window
 *
 * @since 2.4.0
 */
GtkWidget *beasy_dialog_get_action_area(GtkDialog *dialog);

/**
 * Toggles the sensitivity of a widget.
 *
 * @param widget    @c NULL. Used for signal handlers.
 * @param to_toggle The widget to toggle.
 */
void beasy_toggle_sensitive(GtkWidget *widget, GtkWidget *to_toggle);

/**
 * Checks if text has been entered into a GtkTextEntry widget.  If 
 * so, the GTK_RESPONSE_OK on the given dialog is set to TRUE.  
 * Otherwise GTK_RESPONSE_OK is set to FALSE.
 *
 * @param entry  The text entry widget.
 * @param dialog The dialog containing the text entry widget.
 */
void beasy_set_sensitive_if_input(GtkWidget *entry, GtkWidget *dialog);

/**
 * Toggles the sensitivity of all widgets in a pointer array.
 *
 * @param w    @c NULL. Used for signal handlers.
 * @param data The array containing the widgets to toggle.
 */
void beasy_toggle_sensitive_array(GtkWidget *w, GPtrArray *data);

/**
 * Toggles the visibility of a widget.
 *
 * @param widget    @c NULL. Used for signal handlers.
 * @param to_toggle The widget to toggle.
 */
void beasy_toggle_showhide(GtkWidget *widget, GtkWidget *to_toggle);

/**
 * Adds a separator to a menu.
 *
 * @param menu The menu to add a separator to.
 *
 * @return The separator.
 */
GtkWidget *beasy_separator(GtkWidget *menu);

/**
 * Creates a menu item.
 *
 * @param menu The menu to which to append the menu item.
 * @param str  The title to use for the newly created menu item.
 *
 * @return The newly created menu item.
 */
GtkWidget *beasy_new_item(GtkWidget *menu, const char *str);

/**
 * Creates a check menu item.
 *
 * @param menu     The menu to which to append the check menu item.
 * @param str      The title to use for the newly created menu item.
 * @param sf       A function to call when the menu item is activated.
 * @param data     Data to pass to the signal function.
 * @param checked  The initial state of the check item
 *
 * @return The newly created menu item.
 */
GtkWidget *beasy_new_check_item(GtkWidget *menu, const char *str,
		GtkSignalFunc sf, gpointer data, gboolean checked);

/**
 * Creates a menu item.
 *
 * @param menu       The menu to which to append the menu item.
 * @param str        The title for the menu item.
 * @param icon       An icon to place to the left of the menu item,
 *                   or @c NULL for no icon.
 * @param sf         A function to call when the menu item is activated.
 * @param data       Data to pass to the signal function.
 * @param accel_key  Something.
 * @param accel_mods Something.
 * @param mod        Something.
 *
 * @return The newly created menu item.
 */
GtkWidget *beasy_new_item_from_stock(GtkWidget *menu, const char *str,
									const char *icon, GtkSignalFunc sf,
									gpointer data, guint accel_key,
									guint accel_mods, char *mod);

/**
 * Creates a button with the specified text and stock icon.
 *
 * @param text  The text for the button.
 * @param icon  The stock icon name.
 * @param style The orientation of the button.
 *
 * @return The button.
 */
GtkWidget *beasy_pixbuf_button_from_stock(const char *text, const char *icon,
										 BeasyButtonOrientation style);

/**
 * Creates a toolbar button with the stock icon.
 *
 * @param stock The stock icon name.
 *
 * @return The button.
 */
GtkWidget *beasy_pixbuf_toolbar_button_from_stock(const char *stock);

/**
 * Creates a HIG preferences frame.
 *
 * @param parent The widget to put the frame into.
 * @param title  The title for the frame.
 *
 * @return The vbox to put things into.
 */
GtkWidget *beasy_make_frame(GtkWidget *parent, const char *title);

/**
 * Creates a drop-down option menu filled with protocols.
 *
 * @param id        The protocol to select by default.
 * @param cb        The callback to call when a protocol is selected.
 * @param user_data Data to pass to the callback function.
 *
 * @return The drop-down option menu.
 */
GtkWidget *beasy_protocol_option_menu_new(const char *id,
											 GCallback cb,
											 gpointer user_data);

/**
 * Gets the currently selected protocol from a protocol drop down box.
 *
 * @param optmenu The drop-down option menu created by
 *        beasy_account_option_menu_new.
 * @return Returns the protocol ID that is currently selected.
 */
const char *beasy_protocol_option_menu_get_selected(GtkWidget *optmenu);


/**
 * Add autocompletion of screenames to an entry.
 *
 * @deprecated
 *   For new code, use the equivalent:
 *   #beasy_setup_screenname_autocomplete_with_filter(@a entry, @a optmenu,
 *   #beasy_screenname_autocomplete_default_filter, <tt>GINT_TO_POINTER(@a
 *   all)</tt>)
 *
 * @param entry     The GtkEntry on which to setup autocomplete.
 * @param optmenu   A menu for accounts, returned by
 *                  beasy_account_option_menu_new().  If @a optmenu is not @c
 *                  NULL, it'll be updated when a screenname is chosen from the
 *                  autocomplete list.
 * @param all       Whether to include screennames from disconnected accounts.
 */
void beasy_setup_screenname_autocomplete(GtkWidget *entry, GtkWidget *optmenu, gboolean all);

/**
 * Check if the given path is a directory or not.  If it is, then modify
 * the given GtkFileSelection dialog so that it displays the given path.
 * If the given path is not a directory, then do nothing.
 *
 * @param path    The path entered in the file selection window by the user.
 * @param filesel The file selection window.
 *
 * @return TRUE if given path is a directory, FALSE otherwise.
 */
gboolean beasy_check_if_dir(const char *path, GtkFileSelection *filesel);

/**
 * Sets up GtkSpell for the given GtkTextView, reporting errors
 * if encountered.
 *
 * This does nothing if Beasy is not compiled with GtkSpell support.
 *
 * @param textview The textview widget to setup spellchecking for.
 */
void beasy_setup_gtkspell(GtkTextView *textview);

/**
 * Save menu accelerators callback
 */
void beasy_save_accels_cb(GtkAccelGroup *accel_group, guint arg1,
							 GdkModifierType arg2, GClosure *arg3,
							 gpointer data);

/**
 * Save menu accelerators
 */
gboolean beasy_save_accels(gpointer data);

/**
 * Load menu accelerators
 */
void beasy_load_accels(void);

/**
 * Sets an ATK name for a given widget.  Also sets the labelled-by 
 * and label-for ATK relationships.
 *
 * @param w The widget that we want to name.
 * @param l A GtkLabel that we want to use as the ATK name for the widget.
 */
void beasy_set_accessible_label(GtkWidget *w, GtkWidget *l);

/**
 * Sets the labelled-by and label-for ATK relationships.
 *
 * @param w The widget that we want to label.
 * @param l A GtkLabel that we want to use as the label for the widget.
 *
 * @since 2.2.0
 */
void beasy_set_accessible_relations(GtkWidget *w, GtkWidget *l);

/**
 * A helper function for GtkMenuPositionFuncs. This ensures the menu will
 * be kept on screen if possible.
 *
 * @param menu The menu we are positioning.
 * @param x Address of the gint representing the horizontal position
 *        where the menu shall be drawn. This is an output parameter.
 * @param y Address of the gint representing the vertical position
 *        where the menu shall be drawn. This is an output parameter.
 * @param push_in This is an output parameter?
 * @param data Not used by this particular position function.
 *
 * @since 2.1.0
 */
void beasy_menu_position_func_helper(GtkMenu *menu, gint *x, gint *y,
										gboolean *push_in, gpointer data);

/**
 * A valid GtkMenuPositionFunc.  This is used to determine where 
 * to draw context menus when the menu is activated with the 
 * keyboard (shift+F10).  If the menu is activated with the mouse, 
 * then you should just use GTK's built-in position function, 
 * because it does a better job of positioning the menu.
 *
 * @param menu The menu we are positioning.
 * @param x Address of the gint representing the horizontal position
 *        where the menu shall be drawn. This is an output parameter.
 * @param y Address of the gint representing the vertical position
 *        where the menu shall be drawn. This is an output parameter.
 * @param push_in This is an output parameter?
 * @param user_data Not used by this particular position function.
 */
void beasy_treeview_popup_menu_position_func(GtkMenu *menu,
												gint *x,
												gint *y,
												gboolean *push_in,
												gpointer user_data);

/**
 * Creates a status icon for a given primitve
 *
 * @param primitive  The status primitive
 * @param w          The widget to render this
 * @param size       The icon size to render at
 * @return A GdkPixbuf, created from stock
 */
//GdkPixbuf * beasy_create_status_icon(OulStatusPrimitive primitive, GtkWidget *w, const char *size);


/**
 * Append a OulMenuAction to a menu.
 *
 * @param menu    The menu to append to.
 * @param act     The OulMenuAction to append.
 * @param gobject The object to be passed to the action callback.
 *
 * @return   The menuitem added.
 */
GtkWidget *beasy_append_menu_action(GtkWidget *menu, OulMenuAction *act,
                                 gpointer gobject);

/**
 * Sets the mouse pointer for a GtkWidget.
 *
 * After setting the cursor, the display is flushed, so the change will
 * take effect immediately.
 *
 * If the window for @a widget is @c NULL, this function simply returns.
 *
 * @param widget      The widget for which to set the mouse pointer
 * @param cursor_type The type of cursor to set
 */
void beasy_set_cursor(GtkWidget *widget, GdkCursorType cursor_type);

/**
 * Sets the mouse point for a GtkWidget back to that of its parent window.
 *
 * If @a widget is @c NULL, this function simply returns.
 *
 * If the window for @a widget is @c NULL, this function simply returns.
 *
 * @note The display is not flushed from this function.
 */
void beasy_clear_cursor(GtkWidget *widget);

#if !GTK_CHECK_VERSION(2,6,0)
/**
 * Creates a new pixbuf by loading an image from a file. The image will
 * be scaled to fit in the requested size, optionally preserving the image's
 * aspect ratio.
 */
GdkPixbuf *gdk_pixbuf_new_from_file_at_scale(const char *filename, int width, int height,
											 gboolean preserve_aspect_ratio,
											 GError **error);
#endif

/**
 * Converts "->" and "<-" in strings to Unicode arrow characters, for use in referencing
 * menu items.
 *
 * @param str      The text to convert
 * @return         A newly allocated string with unicode arrow characters
 */
char *beasy_make_pretty_arrows(const char *str);

/**
 * The type of callbacks passed to beasy_make_mini_dialog().
 */
typedef void (*BeasyUtilMiniDialogCallback)(gpointer user_data, GtkButton *);

/**
 * This is a callback function to be used for Ctrl+F searching in treeviews.
 * Sample Use:
 * 		gtk_tree_view_set_search_equal_func(treeview,
 * 				beasy_tree_view_search_equal_func,
 * 				search_data, search_data_destroy_cb);
 *
 */
gboolean beasy_tree_view_search_equal_func(GtkTreeModel *model, gint column,
			const gchar *key, GtkTreeIter *iter, gpointer data);

/**
 * Sets or resets a window to 'urgent,' by setting the URGENT hint in X 
 * or blinking in the win32 taskbar
 *
 * @param window  The window to draw attention to
 * @param urgent  Whether to set the urgent hint or not
 */
void beasy_set_urgent(GtkWindow *window, gboolean urgent);

/**
 * Returns TRUE if the GdkPixbuf is opaque, as determined by no
 * alpha at any of the edge pixels.
 *
 * @param pixbuf  The pixbug
 * @return TRUE if the pixbuf is opaque around the edges, FALSE otherwise
 */
gboolean beasy_gdk_pixbuf_is_opaque(GdkPixbuf *pixbuf);

/**
 * Rounds the corners of a 32x32 GdkPixbuf in place
 *
 * @param pixbuf  The buddy icon to transform
 */
void beasy_gdk_pixbuf_make_round(GdkPixbuf *pixbuf);

/**
 * Returns an HTML-style color string for use as a dim grey
 * string
 *
 * @param widget  The widget to return dim grey for
 * @return The dim grey string
 */
const char *beasy_get_dim_grey_string(GtkWidget *widget);

#if !GTK_CHECK_VERSION(2,2,0)
/**
 * This is copied from Gtk to support Gtk 2.0
 *
 * Creates a new path with @a first_index and the varargs as indices.
 *
 * @param first_index    first integer
 * @param ...            list of integers terminated by -1
 *
 * @return               A newly created GtkTreePath.
 */
GtkTreePath *gtk_tree_path_new_from_indices (gint first_index, ...);
#endif

/**
 * Create a simple text GtkComboBoxEntry equivalent
 *
 * @param default_item   Initial contents of GtkEntry
 * @param items          GList containing strings to add to GtkComboBox
 *
 * @return               A newly created text GtkComboBox containing a GtkEntry
 *                       child.
 *
 * @since 2.2.0
 */
GtkWidget *beasy_text_combo_box_entry_new(const char *default_item, GList *items);

/**
 * Retrieve the text from the entry of the simple text GtkComboBoxEntry equivalent
 *
 * @param widget         The simple text GtkComboBoxEntry equivalent widget
 *
 * @return               The text in the widget's entry. It must not be freed
 *
 * @since 2.2.0
 */
const char *beasy_text_combo_box_entry_get_text(GtkWidget *widget);

/**
 * Set the text in the entry of the simple text GtkComboBoxEntry equivalent
 *
 * @param widget         The simple text GtkComboBoxEntry equivalent widget
 * @param text           The text to set
 *
 * @since 2.2.0
 */
void beasy_text_combo_box_entry_set_text(GtkWidget *widget, const char *text);

/**
 * Automatically make a window transient to a suitable parent window.
 *
 * @param window    The window to make transient.
 *
 * @return  Whether the window was made transient or not.
 * @since 2.4.0
 */
gboolean beasy_auto_parent_window(GtkWidget *window);

/**
 * Add a labelled widget to a GtkVBox
 *
 * @param vbox         The GtkVBox to add the widget to.
 * @param widget_label The label to give the widget, can be @c NULL.
 * @param sg           The GtkSizeGroup to add the label to, can be @c NULL.
 * @param widget       The GtkWidget to add.
 * @param expand       Whether to expand the widget horizontally.
 * @param p_label      Place to store a pointer to the GtkLabel, or @c NULL if you don't care.
 *
 * @return  A GtkHBox already added to the GtkVBox containing the GtkLabel and the GtkWidget.
 * @since 2.4.0
 */
GtkWidget *beasy_add_widget_to_vbox(GtkBox *vbox, const char *widget_label, GtkSizeGroup *sg, GtkWidget *widget, gboolean expand, GtkWidget **p_label);

#endif /* _BEASYUTILS_H_ */

