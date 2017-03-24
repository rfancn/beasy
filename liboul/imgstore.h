/**
 * @file imgstore.h IM Image Store API
 * @ingroup core
 * @see @ref imgstore-signals
 */

#ifndef _OUL_IMGSTORE_H_
#define _OUL_IMGSTORE_H_

#include <glib.h>

/** A reference-counted immutable wrapper around an image's data and its
 *  filename.
 */
typedef struct _OulStoredImage OulStoredImage;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add an image to the store.
 *
 * The caller owns a reference to the image in the store, and must dereference
 * the image with oul_imgstore_unref() for it to be freed.
 *
 * No ID is allocated when using this function.  If you need to reference the
 * image by an ID, use oul_imgstore_add_with_id() instead.
 *
 * @param data		Pointer to the image data, which the imgstore will take
 *                      ownership of and free as appropriate.  If you want a
 *                      copy of the data, make it before calling this function.
 * @param size		Image data's size.
 * @param filename	Filename associated with image.  This is for your
 *                  convenience.  It could be the full path to the
 *                  image or, more commonly, the filename of the image
 *                  without any directory information.  It can also be
 *                  NULL, if you don't need to keep track of a filename.
 *
 * @return The stored image.
 */
OulStoredImage *
oul_imgstore_add(gpointer data, size_t size, const char *filename);

/**
 * Add an image to the store, allocating an ID.
 *
 * The caller owns a reference to the image in the store, and must dereference
 * the image with oul_imgstore_unref_by_id() or oul_imgstore_unref()
 * for it to be freed.
 *
 * @param data		Pointer to the image data, which the imgstore will take
 *                      ownership of and free as appropriate.  If you want a
 *                      copy of the data, make it before calling this function.
 * @param size		Image data's size.
 * @param filename	Filename associated with image.  This is for your
 *                  convenience.  It could be the full path to the
 *                  image or, more commonly, the filename of the image
 *                  without any directory information.  It can also be
 *                  NULL, if you don't need to keep track of a filename.

 * @return ID for the image.  This is a unique number that can be used
 *         within liboul to reference the image.
 */
int oul_imgstore_add_with_id(gpointer data, size_t size, const char *filename);

/**
 * Retrieve an image from the store. The caller does not own a
 * reference to the image.
 *
 * @param id		The ID for the image.
 *
 * @return A pointer to the requested image, or NULL if it was not found.
 */
OulStoredImage *oul_imgstore_find_by_id(int id);

/**
 * Retrieves a pointer to the image's data.
 *
 * @param img	The Image
 *
 * @return A pointer to the data, which must not
 *         be freed or modified.
 */
gconstpointer oul_imgstore_get_data(OulStoredImage *img);

/**
 * Retrieves the length of the image's data.
 *
 * @param img	The Image
 *
 * @return The size of the data that the pointer returned by
 *         oul_imgstore_get_data points to.
 */
size_t oul_imgstore_get_size(OulStoredImage *img);

/**
 * Retrieves a pointer to the image's filename.
 *
 * @param img	The image
 *
 * @return A pointer to the filename, which must not
 *         be freed or modified.
 */
const char *oul_imgstore_get_filename(const OulStoredImage *img);

/**
 * Looks at the magic numbers of the image data (the first few bytes)
 * and returns an extension corresponding to the image's file type.
 *
 * @param img  The image.
 *
 * @return The image's extension (for example "png") or "icon"
 *         if unknown.
 */
const char *oul_imgstore_get_extension(OulStoredImage *img);

/**
 * Increment the reference count.
 *
 * @param img The image.
 *
 * @return @a img
 */
OulStoredImage *
oul_imgstore_ref(OulStoredImage *img);

/**
 * Decrement the reference count.
 *
 * If the reference count reaches zero, the image will be freed.
 *
 * @param img The image.
 *
 * @return @a img or @c NULL if the reference count reached zero.
 */
OulStoredImage *
oul_imgstore_unref(OulStoredImage *img);

/**
 * Increment the reference count using an ID.
 *
 * This is a convience wrapper for oul_imgstore_find_by_id() and
 * oul_imgstore_ref(), so if you have a OulStoredImage, it'll
 * be more efficient to call oul_imgstore_ref() directly.
 *
 * @param id		The ID for the image.
 */
void oul_imgstore_ref_by_id(int id);

/**
 * Decrement the reference count using an ID.
 *
 * This is a convience wrapper for oul_imgstore_find_by_id() and
 * oul_imgstore_unref(), so if you have a OulStoredImage, it'll
 * be more efficient to call oul_imgstore_unref() directly.
 *
 * @param id		The ID for the image.
 */
void oul_imgstore_unref_by_id(int id);

/**
 * Returns the image store subsystem handle.
 *
 * @return The subsystem handle.
 */
void *oul_imgstore_get_handle(void);

/**
 * Initializes the image store subsystem.
 */
void oul_imgstore_init(void);

/**
 * Uninitializes the image store subsystem.
 */
void oul_imgstore_uninit(void);

#ifdef __cplusplus
}
#endif

#endif /* _OUL_IMGSTORE_H_ */
