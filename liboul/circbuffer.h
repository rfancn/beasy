#ifndef _CIRCBUFFER_H
#define _CIRCBUFFER_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _OulCircBuffer {

	/** A pointer to the starting address of our chunk of memory. */
	gchar *buffer;

	/** The incremental amount to increase this buffer by when
	 *  the buffer is not big enough to hold incoming data, in bytes. */
	gsize growsize;

	/** The length of this buffer, in bytes. */
	gsize buflen;

	/** The number of bytes of this buffer that contain unread data. */
	gsize bufused;

	/** A pointer to the next byte where new incoming data is
	 *  buffered to. */
	gchar *inptr;

	/** A pointer to the next byte of buffered data that should be
	 *  read by the consumer. */
	gchar *outptr;

} OulCircBuffer;

/**
 * Creates a new circular buffer.  This will not allocate any memory for the
 * actual buffer until data is appended to it.
 *
 * @param growsize The amount that the buffer should grow the first time data
 *                 is appended and every time more space is needed.  Pass in
 *                 "0" to use the default of 256 bytes.
 *
 * @return The new OulCircBuffer. This should be freed with
 *         oul_circ_buffer_destroy when you are done with it
 */
OulCircBuffer *oul_circ_buffer_new(gsize growsize);

/**
 * Dispose of the OulCircBuffer and free any memory used by it (including any
 * memory used by the internal buffer).
 *
 * @param buf The OulCircBuffer to free
 */
void oul_circ_buffer_destroy(OulCircBuffer *buf);

/**
 * Append data to the OulCircBuffer.  This will grow the internal
 * buffer to fit the added data, if needed.
 *
 * @param buf The OulCircBuffer to which to append the data
 * @param src pointer to the data to copy into the buffer
 * @param len number of bytes to copy into the buffer
 */
void oul_circ_buffer_append(OulCircBuffer *buf, gconstpointer src, gsize len);

/**
 * Determine the maximum number of contiguous bytes that can be read from the
 * OulCircBuffer.
 * Note: This may not be the total number of bytes that are buffered - a
 * subsequent call after calling oul_circ_buffer_mark_read() may indicate more
 * data is available to read.
 *
 * @param buf the OulCircBuffer for which to determine the maximum contiguous
 *            bytes that can be read.
 *
 * @return the number of bytes that can be read from the OulCircBuffer
 */
gsize oul_circ_buffer_get_max_read(const OulCircBuffer *buf);

/**
 * Mark the number of bytes that have been read from the buffer.
 *
 * @param buf The OulCircBuffer to mark bytes read from
 * @param len The number of bytes to mark as read
 *
 * @return TRUE if we successfully marked the bytes as having been read, FALSE
 *         otherwise.
 */
gboolean oul_circ_buffer_mark_read(OulCircBuffer *buf, gsize len);

#ifdef __cplusplus
}
#endif

#endif /* _CIRCBUFFER_H */
