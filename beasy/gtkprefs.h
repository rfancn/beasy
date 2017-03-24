#ifndef _BEASY_GTKPREFS_H
#define _BEASY_GTKPREFS_H

#include <gtk/gtk.h>

#include "prefs.h"

#define BEASY_PREFS_ROOT	"/beasy"

G_BEGIN_DECLS

void 		beasy_prefs_show(void);
void 		beasy_prefs_init(void);
GtkWidget *	beasy_prefs_get(void);
void		beasy_prefs_set(GtkWidget * prefs);
GtkWidget * beasy_prefs_checkbox(const gchar *text, const char *key, GtkWidget *page);
GtkWidget * beasy_prefs_dropdown(GtkWidget *box, const gchar *title, 
										OulPrefType type, const char *key, ...);

GtkWidget * beasy_prefs_labeled_spin_button(GtkWidget *box, const gchar *title, 
										const char *key, int min, int max, GtkSizeGroup *sg);

GtkWidget * beasy_prefs_dropdown_from_list(GtkWidget *box, const gchar *title,
								OulPrefType type, const char *key, GList *menuitems);

G_END_DECLS

#endif

