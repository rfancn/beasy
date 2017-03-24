/**
 * @file imgstore.c IM Image Store API
 * @ingroup core
 */
#include <glib.h>
#include "internal.h"

#include "value.h"
#include "debug.h"
#include "imgstore.h"
#include "util.h"

static GHashTable *imgstore;
static unsigned int nextid = 0;

/*
 * NOTE: oul_imgstore_add() creates these without zeroing the memory, so
 * NOTE: make sure to update that function when adding members.
 */
struct _OulStoredImage
{
	int id;
	guint8 refcount;
	size_t size;		/**< The image data's size.	*/
	char *filename;		/**< The filename (for the UI)	*/
	gpointer data;		/**< The image data.		*/
};

OulStoredImage *
oul_imgstore_add(gpointer data, size_t size, const char *filename)
{
	OulStoredImage *img;

	g_return_val_if_fail(data != NULL, NULL);
	g_return_val_if_fail(size > 0, NULL);

	img = g_new(OulStoredImage, 1);
	//OUL_DBUS_REGISTER_POINTER(img, OulStoredImage);
	img->data = data;
	img->size = size;
	img->filename = g_strdup(filename);
	img->refcount = 1;
	img->id = 0;

	return img;
}

int
oul_imgstore_add_with_id(gpointer data, size_t size, const char *filename)
{
	OulStoredImage *img = oul_imgstore_add(data, size, filename);
	if (img) {
		/*
		 * Use the next unused id number.  We do it in a loop on the
		 * off chance that nextid wraps back around to 0 and the hash
		 * table still contains entries from the first time around.
		 */
		do {
			img->id = ++nextid;
		} while (img->id == 0 || g_hash_table_lookup(imgstore, &(img->id)) != NULL);

		g_hash_table_insert(imgstore, &(img->id), img);
	}

	return (img ? img->id : 0);
}

OulStoredImage *oul_imgstore_find_by_id(int id) {
	OulStoredImage *img = g_hash_table_lookup(imgstore, &id);

	if (img != NULL)
		oul_debug_misc("imgstore", "retrieved image id %d\n", img->id);

	return img;
}

gconstpointer oul_imgstore_get_data(OulStoredImage *img) {
	g_return_val_if_fail(img != NULL, NULL);

	return img->data;
}

size_t oul_imgstore_get_size(OulStoredImage *img)
{
	g_return_val_if_fail(img != NULL, 0);

	return img->size;
}

const char *oul_imgstore_get_filename(const OulStoredImage *img)
{
	g_return_val_if_fail(img != NULL, NULL);

	return img->filename;
}

const char *oul_imgstore_get_extension(OulStoredImage *img)
{
	g_return_val_if_fail(img != NULL, NULL);

	return oul_util_get_image_extension(img->data, img->size);
}

void oul_imgstore_ref_by_id(int id)
{
	OulStoredImage *img = oul_imgstore_find_by_id(id);

	g_return_if_fail(img != NULL);

	oul_imgstore_ref(img);
}

void oul_imgstore_unref_by_id(int id)
{
	OulStoredImage *img = oul_imgstore_find_by_id(id);

	g_return_if_fail(img != NULL);

	oul_imgstore_unref(img);
}

OulStoredImage *
oul_imgstore_ref(OulStoredImage *img)
{
	g_return_val_if_fail(img != NULL, NULL);

	img->refcount++;

	return img;
}

OulStoredImage *
oul_imgstore_unref(OulStoredImage *img)
{
	if (img == NULL)
		return NULL;

	g_return_val_if_fail(img->refcount > 0, NULL);

	img->refcount--;

	if (img->refcount == 0)
	{
		oul_signal_emit(oul_imgstore_get_handle(),
		                   "image-deleting", img);
		if (img->id)
			g_hash_table_remove(imgstore, &img->id);

		g_free(img->data);
		g_free(img->filename);
		//OUL_DBUS_UNREGISTER_POINTER(img);
		g_free(img);
		img = NULL;
	}

	return img;
}

void *
oul_imgstore_get_handle()
{
	static int handle;

	return &handle;
}

void
oul_imgstore_init()
{
	void *handle = oul_imgstore_get_handle();

	oul_signal_register(handle, "image-deleting",
	                       oul_marshal_VOID__POINTER, NULL,
	                       1,
	                       oul_value_new(OUL_TYPE_SUBTYPE,
	                                        OUL_SUBTYPE_STORED_IMAGE));

	imgstore = g_hash_table_new(g_int_hash, g_int_equal);
}

void
oul_imgstore_uninit()
{
	g_hash_table_destroy(imgstore);

	oul_signals_unregister_by_instance(oul_imgstore_get_handle());
}
