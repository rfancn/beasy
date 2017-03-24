#ifndef _OUL_INTERNAL_H
#define _OUL_INTERNAL_H

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <glib.h>

/* MAXPATHLEN should only be used with readlink() on glib < 2.4.0.  For
 * anything else, use g_file_read_link() or other dynamic functions.  This is
 * important because Hurd has no hard limits on path length. */
#if !GLIB_CHECK_VERSION(2,4,0)
# ifndef MAXPATHLEN
#  ifdef PATH_MAX
#   define MAXPATHLEN PATH_MAX
#  else
#   define MAXPATHLEN 1024
#  endif
# endif
#endif

#if !GLIB_CHECK_VERSION(2,4,0)
#   define G_MAXUINT32 ((guint32) 0xffffffff)
#endif

#if GLIB_CHECK_VERSION(2,6,0)
#   include <glib/gstdio.h>
#endif

#if !GLIB_CHECK_VERSION(2,6,0)
#   define g_freopen freopen
#   define g_fopen fopen
#   define g_rmdir rmdir
#   define g_remove remove
#   define g_unlink unlink
#   define g_lstat lstat
#   define g_stat stat
#   define g_mkdir mkdir
#   define g_rename rename
#   define g_open open
#endif

#if !GLIB_CHECK_VERSION(2,8,0)
#   define g_access access
#endif

#if !GLIB_CHECK_VERSION(2,10,0)
#   define g_slice_new(type) g_new(type, 1)
#   define g_slice_new0(type) g_new0(type, 1)
#   define g_slice_free(type, mem) g_free(mem)
#endif


/* If we're using NLS, make sure gettext works. If not,
 * then define dummy macros in place of the normal gettext macros.
 *
 * Also, the perl XS config.h file sometimes defines _ So we need to
 * make sure _ isn't already defined before trying to define it.
 * 
 * The Sigular/Plural/Number ngettext dummy definition below was
 * taken from an email to the texinfo mailing list by Manuel Guerrero.
 * Thank you Manuel, and thank you Alex's good friend Google.
 */
#ifndef ENABLE_NLS
#   include <locale.h>
#   include <libintl.h>
#   define _(String) ((const char *)dgettext(PACKAGE, String))
#   ifdef gettext_noop
#       define N_(String) gettext_noop (String)
#   else
#       define N_(String) (String)
#   endif
#else
#   include <locale.h>
#   define N_(String) (String)
#   ifndef _
#       define _(String) ((const char *)String)
#   endif
#   define ngettext(Singular, Plural, Number) ((Number == 1)?((const char *)Singular) : ((const char *)Plural))
#   define dngettext(Domain, Singular, Plural, Number) ((Number == 1) ? ((const char *)Singular) : ((const char *)Plural))
#endif

#ifndef G_GNUC_NULL_TERMINATED
#   if     __GNUC__ >= 4
#       define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
#   else
#       define G_GNUC_NULL_TERMINATED
#   endif
#endif

/***********************************************************
 ******Other header files***********************************
 **********************************************************/
#include <sys/stat.h>
#include <sys/types.h>

#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

/***********************************************************
 ****** macro const definitions ****************************
 **********************************************************/
#define MSG_LEN 2048 
/* we're explicitly asking for the max message
 * length. */
#define BUF_LEN MSG_LEN
#define BUF_LONG BUF_LEN * 2


/***********************************************************
 ****** macro function definitions *************************
 **********************************************************/

/* Safer ways to work with static buffers. When using non-static
 * buffers, either use g_strdup_* functions (preferred) or use
 * g_strlcpy/g_strlcpy directly. */
#define oul_strlcpy(dest, src) g_strlcpy(dest, src, sizeof(dest))
#define oul_strlcat(dest, src) g_strlcat(dest, src, sizeof(dest))

#define OUL_WEBSITE "http://rfan.512j.com/"

#endif
