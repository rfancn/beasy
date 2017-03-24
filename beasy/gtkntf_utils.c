#include <glib.h>
#include <string.h>

#include "internal.h"
#include "gtkntf_utils.h"

gint
gtkntf_utils_strcmp(const gchar *s1, const gchar *s2) {
	gchar *s1k = NULL, *s2k = NULL;
	gint ret = 0;

	if(!s1 && !s2)
		return 0;

	if(!s1)
		return -1;

	if(!s2)
		return 1;

	s1k = g_utf8_collate_key(s1, -1);
	s2k = g_utf8_collate_key(s2, -1);

	ret = strcmp(s1k, s2k);

	g_free(s1k);
	g_free(s2k);

	return ret;
}

gint
gtkntf_utils_compare_strings(gconstpointer a, gconstpointer b) {
	return gtkntf_utils_strcmp(a, b);
}
