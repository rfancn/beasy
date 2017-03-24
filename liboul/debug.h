#ifndef _OUL_DEBUG_H
#define _OUL_DEBUG_H

#include <glib.h>
#include <stdarg.h>

/* Debug levels */
typedef enum {
    OUL_DEBUG_ALL = 0, // All debug levels
    OUL_DEBUG_MISC,    // General chatter
    OUL_DEBUG_INFO,    // General operation info
    OUL_DEBUG_WARN,    // warnings
    OUL_DEBUG_ERROR,   // Errors
    OUL_DEBUG_FATAL    // fatal errors
    
}OulDebugLevel;


#ifdef __cplusplus
extern "C" {
#endif

void        oul_debug_init(void);

void        oul_debug(OulDebugLevel level, const char *category,
                        const char *format, ...) G_GNUC_PRINTF(3, 4);

void        oul_debug_misc(const char *category, const char *format, ...) G_GNUC_PRINTF(2, 3);
void        oul_debug_info(const char *category, const char *format, ...) G_GNUC_PRINTF(2, 3);
void        oul_debug_warning(const char *category, const char *format, ...) G_GNUC_PRINTF(2, 3);
void        oul_debug_error(const char *category, const char *format, ...) G_GNUC_PRINTF(2, 3);
void        oul_debug_fatal(const char *category, const char *format, ...) G_GNUC_PRINTF(2, 3);

void        oul_debug_set_enabled(gboolean enabled);
gboolean    oul_debug_is_enabled();

const char * oul_utf8_strftime(const char *format, const struct tm *tm);


#ifdef __cplusplus
}
#endif

#endif
