#include "internal.h"
#include "beasy.h"
#include "debug.h"
#include "prefs.h"

#include "eggtrayicon.h"
#include "gtkdocklet.h"
#include "beasystock.h"

#define SHORT_EMBED_TIMEOUT 5000

/* globals */
static EggTrayIcon 	*docklet = NULL;
static GtkWidget 	*image = NULL;
static GtkTooltips 	*tooltips = NULL;
static GdkPixbuf 	*blank_icon = NULL;
static int 			embed_timeout = 0;
static int 			docklet_height = 0;

/* protos */
static void docklet_x11_create(gboolean);

static gboolean
docklet_x11_recreate_cb(gpointer data)
{
	docklet_x11_create(TRUE);

	return FALSE; /* for when we're called by the glib idle handler */
}

static void
docklet_x11_embedded_cb(GtkWidget *widget, void *data)
{
	oul_debug(OUL_DEBUG_INFO, "docklet", "embedded\n");
	
	g_source_remove(embed_timeout);
	embed_timeout = 0;
	beasy_docklet_embedded();
}

static void
docklet_x11_destroyed_cb(GtkWidget *widget, void *data)
{
	oul_debug(OUL_DEBUG_INFO, "docklet", "destroyed\n");

	beasy_docklet_remove();

	g_object_unref(G_OBJECT(docklet));
	docklet = NULL;

	g_idle_add(docklet_x11_recreate_cb, NULL);
}

static gboolean
docklet_x11_clicked_cb(GtkWidget *button, GdkEventButton *event, void *data)
{
	if (event->type != GDK_BUTTON_RELEASE)
		return FALSE;

	beasy_docklet_clicked(event->button);
	return TRUE;
}

static void
docklet_x11_update_icon(OulStatusPrimitive status, gboolean connecting, gboolean pending)
{
	const gchar *icon_name = NULL;

	g_return_if_fail(image != NULL);

	switch (status) {
		case OUL_STATUS_OFFLINE:
			icon_name = BEASY_STOCK_TRAY_OFFLINE;
			break;
		case OUL_STATUS_ONLINE:
			icon_name = BEASY_STOCK_TRAY_ONLINE;
			break;
		case OUL_STATUS_BUSY:
			icon_name = BEASY_STOCK_TRAY_BUSY;
			break;
		case OUL_STATUS_UNREAD_MSG:
			icon_name = BEASY_STOCK_TRAY_UNREAD_MSG;
			break;
		default:
			icon_name = BEASY_STOCK_TRAY_OFFLINE;
			break;
	}

	if (pending)
		icon_name = BEASY_STOCK_TRAY_PENDING;
	if (connecting)
		icon_name = BEASY_STOCK_TRAY_CONNECT;

	if(icon_name) {
		int icon_size;
		if (docklet_height < 16)
			icon_size = gtk_icon_size_from_name(BEASY_ICON_SIZE_TANGO_EXTRA_SMALL);
		else if (docklet_height < 32)
			icon_size = gtk_icon_size_from_name(BEASY_ICON_SIZE_TANGO_SMALL);
		else if (docklet_height < 48)
			icon_size = gtk_icon_size_from_name(BEASY_ICON_SIZE_TANGO_MEDIUM);
		else
			icon_size = gtk_icon_size_from_name(BEASY_ICON_SIZE_TANGO_LARGE);

		gtk_image_set_from_stock(GTK_IMAGE(image), icon_name, icon_size);
	}
}

static void
docklet_x11_resize_icon(GtkWidget *widget)
{
	oul_debug(OUL_DEBUG_INFO, "docklet", "docklet given a new space allocation\n");

	if (docklet_height == MIN(widget->allocation.height, widget->allocation.width))
		return;

	docklet_height = MIN(widget->allocation.height, widget->allocation.width);
	oul_debug(OUL_DEBUG_INFO, "docklet", "docklet get the correct height:%d\n", docklet_height);	

	beasy_docklet_update_icon();
}

static void
docklet_x11_blank_icon(void)
{
	if (!blank_icon) {
		GtkIconSize size = GTK_ICON_SIZE_LARGE_TOOLBAR;
		gint width, height;
		g_object_get(G_OBJECT(image), "icon-size", &size, NULL);
		gtk_icon_size_lookup(size, &width, &height);
		blank_icon = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
		gdk_pixbuf_fill(blank_icon, 0);
	}

	gtk_image_set_from_pixbuf(GTK_IMAGE(image), blank_icon);
}

static void
docklet_x11_set_tooltip(gchar *tooltip)
{
	if (!tooltips)
		tooltips = gtk_tooltips_new();

	/* image->parent is a GtkEventBox */
	if (tooltip) {
		gtk_tooltips_enable(tooltips);
		gtk_tooltips_set_tip(tooltips, image->parent, tooltip, NULL);
	} else {
		gtk_tooltips_set_tip(tooltips, image->parent, "", NULL);
		gtk_tooltips_disable(tooltips);
	}
}

#if GTK_CHECK_VERSION(2,2,0)
static void
docklet_x11_position_menu(GtkMenu *menu, int *x, int *y, gboolean *push_in,
						  gpointer user_data)
{
	GtkWidget *widget = GTK_WIDGET(docklet);
	GtkRequisition req;
	gint menu_xpos, menu_ypos;

	gtk_widget_size_request(GTK_WIDGET(menu), &req);
	gdk_window_get_origin(widget->window, &menu_xpos, &menu_ypos);

	menu_xpos += widget->allocation.x;
	menu_ypos += widget->allocation.y;

	if (menu_ypos > gdk_screen_get_height(gtk_widget_get_screen(widget)) / 2)
		menu_ypos -= req.height;
	else
		menu_ypos += widget->allocation.height;

	*x = menu_xpos;
	*y = menu_ypos;

	*push_in = TRUE;
}
#endif

static void
docklet_x11_destroy(void)
{
	g_return_if_fail(docklet != NULL);

	if (embed_timeout)
		g_source_remove(embed_timeout);
	
	beasy_docklet_remove();
	
	g_signal_handlers_disconnect_by_func(G_OBJECT(docklet), G_CALLBACK(docklet_x11_destroyed_cb), NULL);
	gtk_widget_destroy(GTK_WIDGET(docklet));

	g_object_unref(G_OBJECT(docklet));
	docklet = NULL;

	if (blank_icon)
		g_object_unref(G_OBJECT(blank_icon));
	blank_icon = NULL;

	image = NULL;

	oul_debug(OUL_DEBUG_INFO, "docklet", "destroyed\n");
}

static gboolean
docklet_x11_embed_timeout_cb(gpointer data)
{
	/* The docklet was not embedded within the timeout.
	 * Remove it as a visibility manager, but leave the plugin
	 * loaded so that it can embed automatically if/when a notification
	 * area becomes available.
	 */
	oul_debug_info("docklet", "failed to embed within timeout\n");
	beasy_docklet_remove();
	
	return FALSE;
}

static void
docklet_x11_create(gboolean recreate)
{
	GtkWidget *box;

	if (docklet) {
		/* if this is being called when a tray icon exists, it's because
		   something messed up. try destroying it before we proceed,
		   although docklet_refcount may be all hosed. hopefully won't happen. */
		oul_debug(OUL_DEBUG_WARN, "docklet", "trying to create icon but it already exists?\n");
		docklet_x11_destroy();
	}

	docklet = egg_tray_icon_new(BEASY_NAME);
	box = gtk_event_box_new();
	image = gtk_image_new();

	g_signal_connect(G_OBJECT(docklet), "embedded", G_CALLBACK(docklet_x11_embedded_cb), NULL);
	g_signal_connect(G_OBJECT(docklet), "size-allocate", G_CALLBACK(docklet_x11_resize_icon), NULL);
	g_signal_connect(G_OBJECT(docklet), "destroy", 	G_CALLBACK(docklet_x11_destroyed_cb), NULL);
	g_signal_connect(G_OBJECT(box), 	"button-release-event", G_CALLBACK(docklet_x11_clicked_cb), NULL);
	gtk_container_add(GTK_CONTAINER(box), image); 
	gtk_container_add(GTK_CONTAINER(docklet), box);

	if (!gtk_check_version(2,4,0))
		g_object_set(G_OBJECT(box), "visible-window", FALSE, NULL);

	gtk_widget_show_all(GTK_WIDGET(docklet));

	/* ref the docklet before we bandy it about the place */
	g_object_ref(G_OBJECT(docklet));

	if(!recreate) {
		beasy_docklet_embedded();
	    embed_timeout = g_timeout_add(SHORT_EMBED_TIMEOUT, docklet_x11_embed_timeout_cb, NULL);
	}

	oul_debug(OUL_DEBUG_INFO, "docklet", "created\n");

}

static void
docklet_x11_create_ui_op(void)
{
	docklet_x11_create(FALSE);
}

static struct docklet_ui_ops ui_ops =
{
	docklet_x11_create_ui_op,
	docklet_x11_destroy,
	docklet_x11_update_icon,
	docklet_x11_blank_icon,
	docklet_x11_set_tooltip,
#if GTK_CHECK_VERSION(2,2,0)
	docklet_x11_position_menu
#else
	NULL
#endif
};

void
docklet_ui_init()
{
	beasy_docklet_set_ui_ops(&ui_ops);
}
