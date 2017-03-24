#include <stdio.h>

/* MinGW has unistd.h!!! */
#include <unistd.h>

#include "gtkntf_file.h"
#include "internal.h"

gboolean
gtkntf_file_copy_file(const gchar *source, const gchar *destination) {
	FILE *src, *dest;
	gint chr = EOF;

	if(!(src = g_fopen(source, "rb")))
		return FALSE;
	if(!(dest = g_fopen(destination, "wb")))
		return FALSE;

	while((chr = fgetc(src)) != EOF) {
		fputc(chr, dest);
	}

	fclose(dest);
	fclose(src);

	return TRUE;
}

gboolean
gtkntf_file_copy_directory(const gchar *source, const gchar *destination) {
	GDir *dir;
	const gchar *filename;
	gchar *oldfile, *newfile;

	g_return_val_if_fail(source, FALSE);
	g_return_val_if_fail(destination, FALSE);

	dir = g_dir_open(source, 0, NULL);
	if(!dir)
		return FALSE;

	while((filename = g_dir_read_name(dir))) {
		oldfile = g_build_filename(source, filename, NULL);
		newfile = g_build_filename(destination, filename, NULL);

		gtkntf_file_copy_file(oldfile, newfile);

		g_free(oldfile);
		g_free(newfile);
	}

	g_dir_close(dir);

	return TRUE;
}

void
gtkntf_file_remove_dir(const gchar *directory) {
	GDir *dir;
	gchar *path;
	const gchar *file = NULL;

	g_return_if_fail(directory);

	dir = g_dir_open(directory, 0, NULL);

	while((file = g_dir_read_name(dir))) {
		path = g_build_filename(directory, file, NULL);
		g_remove(path);
		g_free(path);
	}

	g_dir_close(dir);
	g_rmdir(directory);
}

gint
gtkntf_file_access(const gchar *filename, gint mode) {
#ifdef _WIN32
	return _access(filename, mode);
#else
	return access(filename, mode);
#endif
}
