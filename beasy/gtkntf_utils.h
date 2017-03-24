#ifndef GTKNTF_UTILS_H
#define GTKNTF_UTILS_H

#include <glib.h>

G_BEGIN_DECLS

gint gtkntf_utils_strcmp(const gchar *s1, const gchar *s2);
gint gtkntf_utils_compare_strings(gconstpointer a, gconstpointer b);

G_END_DECLS

#endif /* GTKNTF_UTILS_H */
