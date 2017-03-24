#ifndef _WIN32
# include <gdk/gdkx.h>
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include <X11/Xatom.h>
#else
# ifndef WINVER
#  define WINVER 0x0500 /* This is Windows 2000 */
# endif /* WINVER */
# include <windows.h>
typedef HMONITOR WINAPI GTKNTF_MonitorFromRect(LPCRECT,DWORD);
typedef BOOL WINAPI GTKNTF_GetMonitorInfo(HMONITOR,LPMONITORINFO);
#endif /* _WIN32 */


#include "internal.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <math.h>
#include <string.h>

#include "debug.h"
#include "prefs.h"
#include "version.h"

#include "gtkntf_action.h"
#include "gtkntf_event_info.h"
#include "gtkntf_notification.h"
#include "gtkpref_notify.h"
#include "gtkntf.h"
#include "gtkntf_display.h"

struct _GtkNtfDisplay {
	/* Widgets */
	GtkWidget *window;
	GtkWidget *event;
	GtkWidget *image;

	/* Display stuff */
	GtkNtfDisplayState state;
	GdkPixbuf *pixbuf;
	GdkRectangle partial;
	gboolean has_alpha;
	gint height;
	gint width;
	gint x;
	gint y;

	/* Timer intervals */
	gint anim_time;
	gint disp_time;

	/* Animation stuff */
	gint round;
	gint rounds;

	/* Action stuff */
	GtkNtfEventInfo *info;
	guint button;
};



/*******************************************************************************
 * consts/macros
 ******************************************************************************/
#define GTKNTF_DISPLAY_ROUND(x)	((x)+0.5)

static const double DELTA_SIZE = 1.333;
static const gint DELTA_TIME = 33.33;

/*******************************************************************************
 * Globals
 ******************************************************************************/
static GList *displays = NULL;
static gboolean vertical, animate;
static GtkNtfDisplayPosition position;

#if GTK_CHECK_VERSION(2,2,0)
static gint disp_screen = 0, disp_monitor = 0;
#endif /* GTK_CHECK_VERSION(2,2,0) */

/*******************************************************************************
 * Prototypes...
 *
 * Yeah, I know they're bad, but sometimes you just can't avoid them...
 *
 * Well, I could, but it'd add more complication...
 ******************************************************************************/
static gboolean gtkntf_display_shown_cb(gpointer data);
static gboolean gtkntf_display_animate_cb(gpointer data);

/*******************************************************************************
 * Callbacks
 ******************************************************************************/
static gboolean
gtkntf_display_button_press_cb(GtkWidget *w, GdkEventButton *e, gpointer data) {
	GtkNtfDisplay *display = GTKNTF_DISPLAY(data);
	gint x = 0, y = 0;

	if(e->type == GDK_BUTTON_PRESS) {
		display->button = e->button;
		return TRUE;
	} else if(e->type == GDK_BUTTON_RELEASE) {
		GtkNtfAction *action = NULL;
		const gchar *pref = NULL;

		gdk_window_get_pointer(w->window, &x, &y, NULL);

		if(x < 0 || x > display->width || y < 0 || y > display->height)
			return FALSE;

		switch(display->button) {
			case 1:		pref = BEASY_PREFS_NTF_MOUSE_LEFT; 		break;
			case 3:		pref = BEASY_PREFS_NTF_MOUSE_RIGHT;		break;
			default:	pref = NULL;					break;
		}

		if(!pref)
			return FALSE;

		action = gtkntf_action_find_with_name(oul_prefs_get_string(pref));
		if(!action)
			return FALSE;

		gtkntf_action_execute(action, display, e);

		return TRUE;
	}

	return FALSE;
}

/*******************************************************************************
 * Uses _NET_WORKAREA or SPI_GETWORKAREA to get the geometry of a screen
 ******************************************************************************/
#ifdef _WIN32
static gboolean
win32_get_workarea_fallback(GdkRectangle *rect) {
	RECT rcWork;
	if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, FALSE)) {
		rect->x = rcWork.left;
		rect->y = rcWork.top;
		rect->width = rcWork.right - rcWork.left;
		rect->height = rcWork.bottom - rcWork.top;

		return TRUE;
	}
	return FALSE;
}

/**
 * This will only work on Win98+ and Win2K+
 */
static gboolean
win32_adjust_workarea_multi_monitor(GdkRectangle *rect) {
	GTKNTF_GetMonitorInfo *the_GetMonitorInfo;
	GTKNTF_MonitorFromRect *the_MonitorFromRect;
	HMODULE hmod;
	HMONITOR monitor;
	MONITORINFO info;
	RECT monitorRect;
	gint virtual_screen_x, virtual_screen_y;

	monitorRect.left = rect->x;
	monitorRect.right = rect->x + rect->width;
	monitorRect.top = rect->y;
	monitorRect.bottom = rect->y + rect->height;

	/* Convert the coordinates so that 0, 0 is the top left of the Primary
	 * Monitor, not the top left of the virtual screen
	 */
	virtual_screen_x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	virtual_screen_y = GetSystemMetrics(SM_YVIRTUALSCREEN);

	if (virtual_screen_x != 0) {
		monitorRect.left += virtual_screen_x;
		monitorRect.right += virtual_screen_x;
	}

	if (virtual_screen_y != 0) {
		monitorRect.top += virtual_screen_y;
		monitorRect.bottom += virtual_screen_y;
	}

	if (!(hmod = GetModuleHandle("user32"))) {
		return FALSE;
	}

	if (!(the_MonitorFromRect = (GTKNTF_MonitorFromRect*)
		GetProcAddress(hmod, "MonitorFromRect"))) {
		return FALSE;
	}

	if (!(the_GetMonitorInfo = (GTKNTF_GetMonitorInfo*)
		GetProcAddress(hmod, "GetMonitorInfoA"))) {
		return FALSE;
	}

	monitor = the_MonitorFromRect(&monitorRect, MONITOR_DEFAULTTONEAREST);

	info.cbSize = sizeof(info);
	if (!the_GetMonitorInfo(monitor, &info)) {
		return FALSE;
	}

	rect->x = info.rcWork.left;
	rect->y = info.rcWork.top;
	rect->width = info.rcWork.right - info.rcWork.left;
	rect->height = info.rcWork.bottom - info.rcWork.top;

	/** Convert the coordinates so that 0, 0 is the top left of the virtual screen,
	 * not the top left of the primary monitor */
	if (virtual_screen_x != 0) {
		rect->x += -virtual_screen_x;
	}
	if (virtual_screen_y != 0) {
		rect->y += -virtual_screen_y;
	}

	return TRUE;
}

static void
win32_adjust_workarea(GdkRectangle *rect) {
	if (!win32_adjust_workarea_multi_monitor(rect)) {
		if (!win32_get_workarea_fallback(rect)) {
			/* I don't think this will ever happen */
			rect->x = 0;
			rect->y = 0;
			rect->height = GetSystemMetrics(SM_CXSCREEN);
			rect->width = GetSystemMetrics(SM_CYSCREEN);
		}
	}
}
#endif /** _WIN32 */

gboolean
gtkntf_display_get_workarea(GdkRectangle *rect) {
#ifndef _WIN32
	Atom xa_desktops, xa_current, xa_workarea, xa_type;
	Display *x_display;
	Window x_root;
	guint32 desktops = 0, current = 0;
	gulong *workareas, len, fill;
	guchar *data;
	gint format;

# if !GTK_CHECK_VERSION(2,2,0)

	x_display = GDK_DISPLAY();
	x_root = XDefaultRootWindow(x_display);

# else /* GTK 2.2.0 and up */

	GdkDisplay *g_display;
	GdkScreen *g_screen;
	Screen *x_screen;

	/* get the gdk display */
	g_display = gdk_display_get_default();
	if(!g_display)
		return FALSE;

	/* get the x display from the gdk display */
	x_display = gdk_x11_display_get_xdisplay(g_display);
	if(!x_display)
		return FALSE;

	/* get the screen according to the prefs */
	g_screen = gdk_display_get_screen(g_display, disp_screen);
	if(!g_screen)
		return FALSE;

	/* get the x screen from the gdk screen */
	x_screen = gdk_x11_screen_get_xscreen(g_screen);
	if(!x_screen)
		return FALSE;

	/* get the root window from the screen */
	x_root = XRootWindowOfScreen(x_screen);

# endif /* !GTK_CHECK_VERSION(2,2,0) */

	/* find the _NET_NUMBER_OF_DESKTOPS atom */
	xa_desktops = XInternAtom(x_display, "_NET_NUMBER_OF_DESKTOPS", True);
	if(xa_desktops == None)
		return FALSE;

	/* get the number of desktops */
	if(XGetWindowProperty(x_display, x_root, xa_desktops, 0, 1, False,
						  XA_CARDINAL, &xa_type, &format, &len, &fill,
						  &data) != Success)
	{
		return FALSE;
	}

	if(!data)
		return FALSE;

	desktops = *(guint32 *)data;
	XFree(data);

	/* find the _NET_CURRENT_DESKTOP atom */
	xa_current = XInternAtom(x_display, "_NET_CURRENT_DESKTOP", True);
	if(xa_current == None)
		return FALSE;

	/* get the current desktop */
	if(XGetWindowProperty(x_display, x_root, xa_current, 0, 1, False,
						  XA_CARDINAL, &xa_type, &format, &len, &fill,
						  &data) != Success)
	{
		return FALSE;
	}

	if(!data)
		return FALSE;

	current = *(guint32 *)data;
	XFree(data);

	/* find the _NET_WORKAREA atom */
	xa_workarea = XInternAtom(x_display, "_NET_WORKAREA", True);
	if(xa_workarea == None)
		return FALSE;

	if(XGetWindowProperty(x_display, x_root, xa_workarea, 0, (glong)(4 * 32),
						  False, AnyPropertyType, &xa_type, &format, &len,
						  &fill, &data) != Success)
	{
		return FALSE;
	}

	/* make sure the type and format are good */
	if(xa_type == None || format == 0)
		return FALSE;

	/* make sure we don't have any leftovers */
	if(fill)
		return FALSE;

	/* make sure len divides evenly by 4 */
	if(len % 4)
		return FALSE;

	/* it's good, lets use it */
	workareas = (gulong *)(guint32 *)data;

	rect->x = (guint32)workareas[current * 4];
	rect->y = (guint32)workareas[current * 4 + 1];
	rect->width = (guint32)workareas[current * 4 + 2];
	rect->height = (guint32)workareas[current * 4 + 3];

	/* clean up our memory */
	XFree(data);
#else
	GdkDisplay *display;
	GdkScreen *screen;

	display = gdk_display_get_default();
	screen = gdk_display_get_screen(display, disp_screen);
	gdk_screen_get_monitor_geometry(screen, disp_monitor, rect);

	win32_adjust_workarea(rect);
#endif /* _WIN32 */

	return TRUE;
}

/*******************************************************************************
 * Gtk 2.0.0 stuff
 *
 * This is the default behavior of 2.0 and 1.x
 ******************************************************************************/
#if !GTK_CHECK_VERSION(2,2,0)
static void
gtkntf_display_get_geometry(gint *x, gint *y, gint *width, gint *height) {
	*x = *y = 0;
	*width = gdk_screen_width();
	*height = gdk_screen_height();
}

static void
gtkntf_display_shape(GtkNtfDisplay *display) {
	if(display->has_alpha) {
		GdkBitmap *bitmap = NULL;
		GdkPixbuf *pixbuf;

		if(display->state == GTKNTF_DISPLAY_STATE_SHOWING ||
		   display->state == GTKNTF_DISPLAY_STATE_HIDING)
		{
			pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(display->image));

			if(!pixbuf) {
				/* image has no pixbuf.. so we have no business continuing */
				return;
			}
		} else {
			pixbuf = display->pixbuf;
		}

		gdk_pixbuf_render_pixmap_and_mask(pixbuf, NULL, &bitmap, 255);
		if(bitmap) {
			gtk_widget_shape_combine_mask(display->window, bitmap, 0, 0);
			g_object_unref(G_OBJECT(bitmap));
		}
	}
}

static void
gtkntf_display_move(GtkNtfDisplay *display) {
	gtk_window_move(GTK_WINDOW(display->window), display->x, display->y);
}

#else /* !GTK_CHECK_VERSION(2,2,0) */
/*******************************************************************************
 * Gtk 2.2.0 and up stuff
 *
 * Allows users to specify which screen to display the notifications on and
 * in the case of xinerama, to position it correctly
 ******************************************************************************/

static void
gtkntf_display_get_geometry(gint *x, gint *y, gint *width, gint *height) {
	GdkDisplay *display;
	GdkScreen *screen;
	GdkRectangle geo, m_geo, w_geo;

	display = gdk_display_get_default();
	screen = gdk_display_get_screen(display, disp_screen);
	gdk_screen_get_monitor_geometry(screen, disp_monitor, &m_geo);

	if(gtkntf_display_get_workarea(&w_geo)) {
		gdk_rectangle_intersect(&w_geo, &m_geo, &geo);
	} else {
		geo.x = m_geo.x;
		geo.y = m_geo.y;
		geo.width = m_geo.width;
		geo.height = m_geo.height;
	}

	*x = geo.x;
	*y = geo.y;
	*width = geo.width;
	*height = geo.height;
}

static void
gtkntf_display_shape(GtkNtfDisplay *display) {
	if(display->has_alpha) {
		GdkBitmap *bmap;
		GdkColormap *cmap;
		GdkPixbuf *pixbuf;
		GdkScreen *screen;

		screen = gdk_display_get_screen(gdk_display_get_default(), disp_screen);
		cmap = gdk_screen_get_system_colormap(screen);

		if(display->state == GTKNTF_DISPLAY_STATE_SHOWING ||
		   display->state == GTKNTF_DISPLAY_STATE_HIDING)
		{
			pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(display->image));

			if(!pixbuf) {
				/* image has no pixbuf.. so we have no business continuing */
				return;
			}
		} else {
			pixbuf = display->pixbuf;
		}

		gdk_pixbuf_render_pixmap_and_mask_for_colormap(pixbuf, cmap, NULL,
													   &bmap, 255);

		if(bmap) {
			gtk_widget_shape_combine_mask(display->window, bmap, 0, 0);
			g_object_unref(G_OBJECT(bmap));
		}
	}
}

static void
gtkntf_display_move(GtkNtfDisplay *display) {
	GdkScreen *screen_t, *screen_s;

	screen_t = gdk_display_get_screen(gdk_display_get_default(), disp_screen);
	screen_s = gtk_window_get_screen(GTK_WINDOW(display->window));

	if(gdk_screen_get_number(screen_s) != gdk_screen_get_number(screen_t)) {
		if(display->has_alpha)
			gtk_widget_shape_combine_mask(display->window, NULL, 0, 0);

		gtk_window_set_screen(GTK_WINDOW(display->window), screen_t);

		if(display->has_alpha)
			gtkntf_display_shape(display);
	}

	gtk_window_move(GTK_WINDOW(display->window), display->x, display->y);
}

gint
gtkntf_display_get_default_screen() {
	GdkScreen *screen;

	screen = gdk_screen_get_default();

	return gdk_screen_get_number(screen);
}

gint
gtkntf_display_get_screen_count() {
	return gdk_display_get_n_screens(gdk_display_get_default()) - 1;
}

gint
gtkntf_display_get_default_monitor() {
	return 0;
}

gint
gtkntf_display_get_monitor_count() {
	GdkDisplay *display;
	GdkScreen *screen = NULL;
	gint screens = 0, monitors = 0, i = 0;

	display = gdk_display_get_default();
	screens = gdk_display_get_n_screens(display);

	for(i = 0; i < screens; i++) {
		screen = gdk_display_get_screen(display, i);

		monitors = MAX(monitors, gdk_screen_get_n_monitors(screen));
	}

	return monitors - 1;
}

#endif /* GTK_CHECK_VERSION(2,2,0) */

/*******************************************************************************
 * The normal code (tm)
 ******************************************************************************/
static void
gtkntf_display_position(GtkNtfDisplay *new_display) {
	GtkNtfDisplay *display;
	GList *l = NULL;
	gint pad_x = 0, pad_y = 0;
	gint disp_width = 0, disp_height = 0;
	gint width = 0, height = 0;
	gint total = 0;

	g_return_if_fail(new_display);

	gtkntf_display_get_geometry(&pad_x, &pad_y, &width, &height);

	for(l = displays; l; l = l->next) {
		display = GTKNTF_DISPLAY(l->data);

		if(display == new_display)
			break;

		if(vertical)
			total += display->height;
		else
			total += display->width;
	}

	if(new_display->state == GTKNTF_DISPLAY_STATE_SHOWING ||
	   new_display->state == GTKNTF_DISPLAY_STATE_HIDING)
	{
		disp_width = new_display->partial.width;
		disp_height = new_display->partial.height;
	} else {
		disp_width = new_display->width;
		disp_height = new_display->height;
	}

	/* set the correct size for the window */
	gtk_widget_set_size_request(new_display->window, disp_width, disp_height);

	switch(position) {
		case GTKNTF_DISPLAY_POSITION_NW:
			if(vertical) {
				new_display->x = pad_x;
				new_display->y = pad_y + total;
			} else {
				new_display->x = pad_x + total;
				new_display->y = pad_y;
			}

			break;
		case GTKNTF_DISPLAY_POSITION_NE:
			if(vertical) {
				new_display->x = pad_x + width - disp_width;
				new_display->y = pad_y + total;
			} else {
				new_display->x = pad_x + width - (total + disp_width);
				new_display->y = pad_y;
			}

			break;
		case GTKNTF_DISPLAY_POSITION_SW:
			if(vertical) {
				new_display->x = pad_x;
				new_display->y = pad_y + height - (total + disp_height);
			} else {
				new_display->x = pad_x + total;
				new_display->y = pad_y + height - (disp_height);
			}

			break;
		case GTKNTF_DISPLAY_POSITION_SE:
			if(vertical) {
				new_display->x = pad_x + width - disp_width;
				new_display->y = pad_y + height - (total + disp_height);
			} else {
				new_display->x = pad_x + width - (total + disp_width);
				new_display->y = pad_y + height - disp_height;
			}

			break;
		default:
			break;
	}

	gtkntf_display_move(new_display);
}

static void
gtkntf_displays_position() {
	GtkNtfDisplay *display;
	GList *l;

	for(l = displays; l; l = l->next) {
		display = GTKNTF_DISPLAY(l->data);

		gtkntf_display_position(display);
	}
}

GtkNtfDisplay *
gtkntf_display_new() {
	GtkNtfDisplay *display;

	display = g_new0(GtkNtfDisplay, 1);

	return display;
}

void
gtkntf_display_destroy(GtkNtfDisplay *display) {
	g_return_if_fail(display);

	displays = g_list_remove(displays, display);

	if(display->window) {
		gtk_widget_destroy(display->window);
		display->window = NULL;
	}

	if(display->pixbuf) {
		g_object_unref(G_OBJECT(display->pixbuf));
		display->pixbuf = NULL;
	}

	if(display->info) {
		gtkntf_event_info_destroy(display->info);
		display->info = NULL;
	}

	g_free(display);
	display = NULL;

	gtkntf_displays_position();
}

static gboolean
gtkntf_display_shown_cb(gpointer data) {
	GtkNtfDisplay *display = GTKNTF_DISPLAY(data);
	guint timeout_id;

	/* in case something wicked happened */
	g_return_val_if_fail(display, FALSE);

	display->state = GTKNTF_DISPLAY_STATE_HIDING;

	timeout_id = g_timeout_add(DELTA_TIME, gtkntf_display_animate_cb, data);
	gtkntf_event_info_set_timeout_id(display->info, timeout_id);

	return FALSE;
}

static gint
gtkntf_display_calculate_change(GtkNtfDisplay *display, gint maximum) {
	/* This function calculates an an exponential change.
	 *
	 * It's a basic scalable exponential movement formula, given to us by
	 * Nathaniel Waters.
	 */
	gint ret;

	/* this could probably use some type casting somewhere.. */
	ret = GTKNTF_DISPLAY_ROUND(((gdouble)maximum /
		  pow(DELTA_SIZE, display->rounds)) *
		  pow(DELTA_SIZE, display->round));

	return ret;
}

static gboolean
gtkntf_display_animate(GtkNtfDisplay *display) {
	GdkPixbuf *pixbuf;
	GdkRectangle full;
	gint total, current;
	gboolean ret = TRUE;

	/* figure out what we're modifing */
	if(vertical)
		total = display->height;
	else
		total = display->width;

	/* calculate the change */
	current = gtkntf_display_calculate_change(display, total);

	/* create our rects */
	full.x = 0;
	full.y = 0;
	full.width = display->width;
	full.height = display->height;

	/* ugh too many possibilities */
	switch(position) {
		case GTKNTF_DISPLAY_POSITION_NW:
			if(vertical) {
				display->partial.x = full.x;
				display->partial.y = full.y;
				display->partial.width = full.width;
				display->partial.height = current;
			} else {
				display->partial.x = full.width - current;
				display->partial.y = full.y;
				display->partial.width = current;
				display->partial.height = full.height;
			}

			break;
		case GTKNTF_DISPLAY_POSITION_NE:
			if(vertical) {
				display->partial.x = full.x;
				display->partial.y = full.y;
				display->partial.width = full.width;
				display->partial.height = current;
			} else {
				display->partial.x = full.x;
				display->partial.y = full.y;
				display->partial.width = current;
				display->partial.height = full.height;
			}

			break;
		case GTKNTF_DISPLAY_POSITION_SW:
			if(vertical) {
				display->partial.x = full.x;
				display->partial.y = full.y;
				display->partial.width = full.width;
				display->partial.height = current;
			} else {
				display->partial.x = full.width - current;
				display->partial.y = full.y;
				display->partial.width = current;
				display->partial.height = full.height;
			}

			break;
		case GTKNTF_DISPLAY_POSITION_SE:
			if(vertical) {
				display->partial.x = full.x;
				display->partial.y = full.y;
				display->partial.width = full.width;
				display->partial.height = current;
			} else {
				display->partial.x = full.x;
				display->partial.y = full.y;
				display->partial.width = current;
				display->partial.height = full.height;
			}

			break;
		default:
			display->partial.x = full.x;
			display->partial.y = full.y;
			display->partial.width = full.width;
			display->partial.height = full.height;

			break;
	}

	/* Add some sanity checks */
	if(display->partial.width <= 0)
		display->partial.width = 1;
	if(display->partial.height <= 0)
		display->partial.height = 1;

	/* create our partial pixbuf and fill it */
	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, display->has_alpha,	8,
							display->partial.width, display->partial.height);

	if(!pixbuf) {
		oul_debug_info("guifications", "failed to created partial pixbuf, "
						"destroying display %p\n", display);
		gtkntf_display_destroy(display);

		return FALSE;
	}

	gdk_pixbuf_copy_area(display->pixbuf,
						 display->partial.x, display->partial.y,
						 display->partial.width, display->partial.height,
						 pixbuf, 0, 0);

	/* force the image and event box to be the same size as the partial
	 * pixbuf
	 */
	gtk_widget_set_size_request(display->image, display->partial.width,
								display->partial.height);
	gtk_widget_set_size_request(display->event, display->partial.width,
								display->partial.height);

	/* update the image's pixbuf */
	gtk_image_set_from_pixbuf(GTK_IMAGE(display->image), pixbuf);

	/* unref the partial pixbuf */
	g_object_unref(G_OBJECT(pixbuf));

	/* update the display and force gdk to redraw it */
	gtkntf_display_shape(display);
	gtkntf_display_position(display);
	gdk_window_process_updates(GDK_WINDOW(display->window->window), TRUE);

	/* Finish up */
	if(display->state == GTKNTF_DISPLAY_STATE_SHOWING) {
		display->round++;

		if(display->round > display->rounds) {
			guint timeout_id;

			/* set rounds back to the max */
			display->round = display->rounds - 1;

			display->state = GTKNTF_DISPLAY_STATE_SHOWN;

			timeout_id = gtk_timeout_add(display->disp_time,
										 gtkntf_display_shown_cb,
										 (gpointer)display);
			gtkntf_event_info_set_timeout_id(display->info, timeout_id);

			ret = FALSE;
		}
	} else { /* if(display->state == GTKNTF_DISPLAY_STATE_HIDING) { */
		display->round--;

		if(display->round <= 0) {
			gtkntf_display_destroy(display);

			ret = FALSE;
		}
	}

	return ret;
}

static gboolean
gtkntf_display_animate_cb(gpointer data) {
	GtkNtfDisplay *display = GTKNTF_DISPLAY(data);
	gboolean ret;

	/* in the event something wicked happened... */
	g_return_val_if_fail(display, FALSE);

	/* we call a function to do our animation here to modularize the process
	 * so we can do other types of animation, like fading depending on other
	 * conditions.  These conditions should be checked here, and then called
	 * accordingly.
	 *
	 * The animation function should return a boolean so we can tell the
	 * timer handler whether or not to keep the timer around.
	 */
	ret = gtkntf_display_animate(display);

	return ret;
}

static gboolean
gtkntf_display_destroy_cb(gpointer data) {
	GtkNtfDisplay *display = GTKNTF_DISPLAY(data);

	gtkntf_display_destroy(display);

	return FALSE;
}

/* Yes this big ugly function has a purpose...
 * It checks for existing notification from the same plugin, and condenses it.
 *
 * It returns TRUE if the new notification/event should be destroyed because
 * theres a matching notification that has a higher priority than the new
 * notification/event
 */
static gboolean
gtkntf_display_condense(GtkNtfEventInfo *info) {
	GtkNtfDisplay *display = NULL;
	GtkNtfEvent *event1 = NULL, *event2 = NULL;
	GtkNtfEventPriority priority1, priority2;
	GList *l = NULL, *ll = NULL;
	gchar *ck1 = NULL, *ck2 = NULL;
	gboolean ret = FALSE;

	event1 = gtkntf_event_info_get_event(info);
	priority1 = gtkntf_event_get_priority(event1);

	for(l = displays; l; l = ll) {
		display = GTKNTF_DISPLAY(l->data);
		ll = l->next;

		event2 = gtkntf_event_info_get_event(display->info);
		priority2 = gtkntf_event_get_priority(event2);

	}

	if(ck1)
		g_free(ck1);

	/* we only check the stack count if ret is true, because that means one
	 * has been removed, so we have space.
	 */
	if(ret == FALSE) {
		gint throttle = oul_prefs_get_int(BEASY_PREFS_NTF_BEHAVIOR_THROTTLE);

		if(throttle > 0 && g_list_length(displays) + 1 > throttle) {
			display = GTKNTF_DISPLAY(g_list_nth_data(displays, 0));

			if(display)
				gtkntf_display_destroy(display);

			gtkntf_displays_position();
		}
	}

	return ret;
}

gboolean
gtkntf_display_screen_saver_is_running() {
	gboolean ret = FALSE;

	static Atom xss, locked, blanked;
	static gboolean init = FALSE;
	Atom ratom;
	gint rtatom;
	guchar *data = NULL;
	gulong items, padding;

	if(!init) {
		xss = XInternAtom(GDK_DISPLAY(),"_SCREENSAVER_STATUS", FALSE);
		locked = XInternAtom(GDK_DISPLAY(), "LOCK", FALSE);
		blanked = XInternAtom(GDK_DISPLAY(), "BLANK", FALSE);
		init = TRUE;
	}

	if(XGetWindowProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(), xss, 0, 999, FALSE,
						  XA_INTEGER, &ratom, &rtatom, &items, &padding, &data)
						  == Success)
	{
		if(ratom == XA_INTEGER || items >= 3) {
			guint *item_data = (guint *)data;

			if(item_data[0] == locked || item_data[0] == blanked)
				ret = TRUE;
		}
		XFree(data);
	}

	return ret;
}

void
gtkntf_display_show_event(GtkNtfEventInfo *info, GtkNtfNotification *notification) {
	GtkNtfDisplay *display = NULL;
	gint display_time;
	guint timeout_id = 0;

	g_return_if_fail(info);

	/* Here we kill the notification if the screen saver is running */
	if(gtkntf_display_screen_saver_is_running()) {
		gtkntf_event_info_destroy(info);
		return;
	}

	/* we should never make it here with out a notification, but just in 
	 * case we do....
	 */
	if(!notification) {
		const gchar *event_name = gtkntf_event_get_name(gtkntf_event_info_get_event(info));

		oul_debug_info("GuiNotifications",
						"could not find a notification for the event \"%s\"\n",
						event_name ? event_name : "");
		return;
	}

	/* condense contacts and same buddy notifications, a return value of TRUE
	 * means theres a higher priority notification already being displayed
	 */
	if(gtkntf_display_condense(info)) {
		gtkntf_event_info_destroy(info);
		return;
	}

	/* create the display */
	display = gtkntf_display_new();
	display->info = info;

	/* Render the pixbuf, if we get NULL destroy the display and return */
	display->pixbuf = gtkntf_notification_render(notification, display->info);
	if(!display->pixbuf) {
		GtkNtfTheme *theme = gtkntf_notification_get_theme(notification);
		GtkNtfThemeInfo *info = gtkntf_theme_get_theme_info(theme);

		oul_debug_info("GTKNotify", "render '%s' failed for theme '%s'\n",
						gtkntf_notification_get_type(notification), 
						gtkntf_theme_info_get_name(info));
		gtkntf_display_destroy(display);
		return;
	}

	/* grab info about the pixbuf */
	display->has_alpha = gdk_pixbuf_get_has_alpha(display->pixbuf);
	display->height = gdk_pixbuf_get_height(display->pixbuf);
	display->width = gdk_pixbuf_get_width(display->pixbuf);

	/* if we've made it this far, we can create the window safely */
	display->window = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_role(GTK_WINDOW(display->window), "guification");

	/* Create the event box, you know, so the mouse works! */
	display->event = gtk_event_box_new();
	if(!gtk_check_version(2,4,0))
		g_object_set(G_OBJECT(display->event), "visible-window", FALSE, NULL);
	gtk_container_add(GTK_CONTAINER(display->window), display->event);

    /* now we create all our mouse signals that should play nice with
	 * other apps..
	 */
	g_signal_connect(G_OBJECT(display->window), "button-press-event",
					 G_CALLBACK(gtkntf_display_button_press_cb),
					 (gpointer)display);
	g_signal_connect(G_OBJECT(display->window), "button-release-event",
					 G_CALLBACK(gtkntf_display_button_press_cb),
					 (gpointer)display);

	/* create the image but don't put the pixbuf in it quite yet. */
	display->image = gtk_image_new();
	gtk_container_add(GTK_CONTAINER(display->event), display->image);

	/* grab the display time */
	display_time = 1000 * oul_prefs_get_int(BEASY_PREFS_NTF_BEHAVIOR_DISPLAY_TIME);

	/* animation is set, this is a new notification, so we animate it */
	if(animate) {
		/* we explicitly set the size request because the window is created
		 * with an image with no child, so it defaults to some insane size.
		 */
		gtk_widget_set_size_request(display->window,
									display->width, display->height);

		/* calculate our timer intervals.  anim_time is used for showing and
		 * hiding, so that's 2/8ths which is 1/4th.
		 *
		 * Remember display time is stored in seconds!
		 */
		display->anim_time = display_time / 8;
		display->disp_time = display_time * 3 / 4;

		/* Calculate and store some more information about animating */
		display->rounds = GTKNTF_DISPLAY_ROUND((gfloat)display->anim_time / 
										   (gfloat)DELTA_TIME);
		display->round = 0;

		/* Set the state of the display */
		display->state = GTKNTF_DISPLAY_STATE_SHOWING;

		/* setup a timeout of DELTA_TIME ms for animating */
		timeout_id = g_timeout_add(DELTA_TIME, gtkntf_display_animate_cb,
								   (gpointer)display);
	} else { /* no animation! */
		/* set the image since we're not animating */
		gtk_image_set_from_pixbuf(GTK_IMAGE(display->image), display->pixbuf);

		/* shape it */
		gtkntf_display_shape(display);

		/* We're showing! */
		display->state = GTKNTF_DISPLAY_STATE_SHOWN;

		/* build our timeout */
		timeout_id = g_timeout_add(display_time, gtkntf_display_destroy_cb,
								   (gpointer)display);
	}

	gtkntf_event_info_set_timeout_id(info, timeout_id);

	/* position and show the widget */
	gtkntf_display_position(display);
	gtk_widget_show_all(display->window);

	/* add the display to the list */
	displays = g_list_append(displays, display);
}

GtkNtfEventInfo *
gtkntf_display_get_event_info(GtkNtfDisplay *display) {
	g_return_val_if_fail(display, NULL);

	return display->info;
}

static void
gtkntf_display_position_changed_cb(const gchar *name, OulPrefType type,
							   gconstpointer val, gpointer data)
{
	vertical = oul_prefs_get_bool(BEASY_PREFS_NTF_APPEARANCE_VERTICAL);
	position = oul_prefs_get_int(BEASY_PREFS_NTF_APPEARANCE_POSITION);

    gtkntf_displays_position();
}

static void
gtkntf_display_animate_changed_cb(const gchar *name, OulPrefType type,
							  gconstpointer val, gpointer data)
{
	animate = GPOINTER_TO_INT(val);
}

/*******************************************************************************
 * Pref callbacks for the display/screen/monitor stuff
 ******************************************************************************/
#if GTK_CHECK_VERSION(2,2,0)
static void
gtkntf_display_screen_changed_cb(const gchar *name, OulPrefType type,
							 gconstpointer val, gpointer data)
{
	disp_screen = GPOINTER_TO_INT(val);
	gtkntf_item_text_uninit();
	gtkntf_displays_position();
	gtkntf_item_text_init();
}

static void
gtkntf_display_monitor_changed_cb(const gchar *name, OulPrefType type,
							  gconstpointer val, gpointer data)
{
	disp_monitor = GPOINTER_TO_INT(val);
	gtkntf_displays_position();
}

static guint scr_chg_id = 0, mon_chg_id = 0;
#endif /* GTK_CHECK_VERSION(2,2,0) */

/*******************************************************************************
 * Regular pref callbacks
 ******************************************************************************/
static guint pos_chg_id = 0, ver_chg_id = 0, ani_chg_id = 0;

void
gtkntf_display_init(void) {
	void *handle = gtkntf_get_handle();
	
	/* since we just use the callbacks to get the changes we need to know
	 * what these are initially at.
	 */
	position = oul_prefs_get_int(BEASY_PREFS_NTF_APPEARANCE_POSITION);
	vertical = oul_prefs_get_bool(BEASY_PREFS_NTF_APPEARANCE_VERTICAL);
	animate = oul_prefs_get_bool(BEASY_PREFS_NTF_APPEARANCE_ANIMATE);

	/* Connect our pref callbacks */
	pos_chg_id = oul_prefs_connect_callback(handle,
										   BEASY_PREFS_NTF_APPEARANCE_POSITION,
										   gtkntf_display_position_changed_cb,
										   NULL);
	ver_chg_id = oul_prefs_connect_callback(handle,
										   BEASY_PREFS_NTF_APPEARANCE_VERTICAL,
										   gtkntf_display_position_changed_cb,
										   NULL);
	ani_chg_id = oul_prefs_connect_callback(handle,
										   BEASY_PREFS_NTF_APPEARANCE_ANIMATE,
										   gtkntf_display_animate_changed_cb,
										   NULL);

#if GTK_CHECK_VERSION(2,2,0)
	/* setup our multi screen prefs */
	disp_screen = oul_prefs_get_int(BEASY_PREFS_NTF_ADVANCED_SCREEN);
	disp_monitor = oul_prefs_get_int(BEASY_PREFS_NTF_ADVANCED_MONITOR);

	scr_chg_id = oul_prefs_connect_callback(handle,
										   BEASY_PREFS_NTF_ADVANCED_SCREEN,
										   gtkntf_display_screen_changed_cb,
										   NULL);
	mon_chg_id = oul_prefs_connect_callback(handle,
										   BEASY_PREFS_NTF_ADVANCED_MONITOR,
										   gtkntf_display_monitor_changed_cb,
										   NULL);
#endif /* GTK_CHECK_VERSION(2,2,0) */
}

void
gtkntf_display_uninit() {
	oul_prefs_disconnect_callback(pos_chg_id);
	oul_prefs_disconnect_callback(ver_chg_id);
#if GTK_CHECK_VERSION(2,2,0)
	oul_prefs_disconnect_callback(scr_chg_id);
	oul_prefs_disconnect_callback(mon_chg_id);
#endif /* GTK_CHECK_VERSION(2,2,0) */
}
