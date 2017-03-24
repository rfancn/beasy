#ifndef GTKNTF_FILE_H
#define GTKNTF_FILE_H

#include <glib.h>

G_BEGIN_DECLS

gboolean 	gtkntf_file_copy_file(const gchar *source, const gchar *dest);
gboolean 	gtkntf_file_copy_directory(const gchar *source, const gchar *destination);
void 		gtkntf_file_remove_dir(const gchar *directory);
gint 		gtkntf_file_access(const gchar *filename, gint mode);

G_END_DECLS

#endif /* GTKNTF_FILE_H */
