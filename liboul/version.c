#include "internal.h"

#include "version.h"

const guint oul_major_version = OUL_MAJOR_VERSION;
const guint oul_minor_version = OUL_MINOR_VERSION;
const guint oul_micro_version = OUL_MICRO_VERSION;

const char *oul_version_check(guint required_major, guint required_minor, guint required_micro)
{
	if (required_major > OUL_MAJOR_VERSION)
		return "liboul version too old (major mismatch)";
	if (required_major < OUL_MAJOR_VERSION)
		return "liboul version too new (major mismatch)";
	if (required_minor > OUL_MINOR_VERSION)
		return "liboul version too old (minor mismatch)";
	if ((required_minor == OUL_MINOR_VERSION) && (required_micro > OUL_MICRO_VERSION))
		return "liboul version too old (micro mismatch)";
	return NULL;
}
