#include "internal.h"

#include "debug.h"

static gboolean debug_enabled = FALSE;

/******************************************************
 **********static functiosn****************************
 *****************************************************/
static void
oul_debug_vargs(OulDebugLevel level, const char *category,
    const char *format, va_list args)
{
    char *arg_str = NULL;

    g_return_if_fail(level != OUL_DEBUG_ALL );
    g_return_if_fail(format != NULL);

    if(!debug_enabled) return;

    arg_str = g_strdup_vprintf(format, args);
    if(debug_enabled){
        gchar *ts_str;
        const char *mdate;
        time_t mtime = time(NULL);
        
        mdate = oul_utf8_strftime("%H:%M:%S", localtime(&mtime)); 
        ts_str = g_strdup_printf("(%s) ", mdate);
        
        if(category == NULL)
            g_print("%s%s", ts_str, arg_str);
        else
            g_print("%s%s: %s", ts_str, category, arg_str);

        g_free(ts_str);
    }
        
    g_free(arg_str); 
}

/******************************************************
 **********public functiosn****************************
 *****************************************************/
void
oul_debug(OulDebugLevel level, const char *category, 
        const char *format, ...)
{
    va_list args;
    g_return_if_fail(level != OUL_DEBUG_ALL);
    g_return_if_fail(format != NULL);

    va_start(args, format);
    oul_debug_vargs(level, category, format, args);
    va_end(args); 
}

void
oul_debug_misc(const char *category, const char *format, ...)
{
    va_list args;
    
    g_return_if_fail(format != NULL);

    va_start(args, format);
    oul_debug_vargs(OUL_DEBUG_MISC, category, format, args);
    va_end(args);
}

void
oul_debug_info(const char *category, const char *format, ...)
{
    va_list args;
    
    g_return_if_fail(format != NULL);

    va_start(args, format);
    oul_debug_vargs(OUL_DEBUG_INFO, category, format, args);
    va_end(args);
}

void
oul_debug_warning(const char *category, const char *format, ...)
{
    va_list args;
    
    g_return_if_fail(format != NULL);

    va_start(args, format);
    oul_debug_vargs(OUL_DEBUG_WARN, category, format, args);
    va_end(args);
}

void
oul_debug_error(const char *category, const char *format, ...)
{
    va_list args;
    
    g_return_if_fail(format != NULL);

    va_start(args, format);
    oul_debug_vargs(OUL_DEBUG_ERROR, category, format, args);
    va_end(args);
}

void
oul_debug_fatal(const char *category, const char *format, ...)
{
    va_list args;
    
    g_return_if_fail(format != NULL);

    va_start(args, format);
    oul_debug_vargs(OUL_DEBUG_FATAL, category, format, args);
    va_end(args);
}

void
oul_debug_set_enabled(gboolean enabled)
{
    debug_enabled = enabled;
}

gboolean
oul_debug_is_enabled(void)
{
    return debug_enabled; 
}
