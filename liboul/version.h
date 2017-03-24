#ifndef _OUL_VERSION_H_
#define _OUL_VERSION_H_

#define OUL_MAJOR_VERSION (0)
#define OUL_MINOR_VERSION (0)
#define OUL_MICRO_VERSION (1)

#define OUL_VERSION_CHECK(x,y,z) ((x) == OUL_MAJOR_VERSION && \
									 ((y) < OUL_MINOR_VERSION || \
									  ((y) == OUL_MINOR_VERSION && (z) <= OUL_MICRO_VERSION)))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Checks that the liboul version is compatible with the requested
 * version
 *
 * @param required_major: the required major version.
 * @param required_minor: the required minor version.
 * @param required_micro: the required micro version.
 *
 * @return NULL if the versions are compatible, or a string describing
 *         the version mismatch if not compatible.
 */
const char *oul_version_check(guint required_major, guint required_minor, guint required_micro);

/**
 * The major version of the running liboul.  Contrast with
 * #OUL_MAJOR_VERSION, which expands at compile time to the major version of
 * liboul being compiled against.
 *
 * @since 2.4.0
 */
extern const guint oul_major_version;

/**
 * The minor version of the running liboul.  Contrast with
 * #OUL_MINOR_VERSION, which expands at compile time to the minor version of
 * liboul being compiled against.
 *
 * @since 2.4.0
 */
extern const guint oul_minor_version;

/**
 *
 * The micro version of the running liboul.  Contrast with
 * #OUL_MICRO_VERSION, which expands at compile time to the micro version of
 * liboul being compiled against.
 *
 * @since 2.4.0
 */
extern const guint oul_micro_version;

#ifdef __cplusplus
}
#endif

#endif /* _OUL_VERSION_H_ */

