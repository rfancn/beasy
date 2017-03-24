#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# UPDATING VERSION NUMBERS FOR RELEASES
#
# oul_micro_version += 1
#
# If any functions have been added to liboul:
#   oul_micro_version = 0
#   oul_minor_version += 1
#   oul_lt_current += 1
#
# If backwards compatibility has been broken in libpurple, Pidgin, or Finch:
#   oul_micro_version = 0
#   oul_minor_version = 0
#   oul_major_version += 1;
#   oul_lt_current += 1
#
AC_PREREQ(2.59)

AC_INIT([beasy], [0.2], [rfan.cn@gmail.com])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE(beasy, 0.2)

m4_define([oul_lt_current], [0])
m4_define([oul_major_version], [0])
m4_define([oul_minor_version], [0])
m4_define([oul_micro_version], [1])

OUL_MAJOR_VERSION=oul_major_version
OUL_MINOR_VERSION=oul_minor_version
OUL_MICRO_VERSION=oul_micro_version
AC_SUBST(OUL_MAJOR_VERSION)
AC_SUBST(OUL_MINOR_VERSION)
AC_SUBST(OUL_MICRO_VERSION)


EASY_LT_VERSION_INFO="oul_lt_current:oul_micro_version:oul_minor_version"
AC_SUBST(EASY_LT_VERSION_INFO)

# Checks for programs.
AC_PROG_CC
AM_PROG_LIBTOOL
LIBTOOL="$LIBTOOL --silent"
PKG_PROG_PKG_CONFIG

GETTEXT_PACKAGE=beasy
AC_SUBST(GETTEXT_PACKAGE)

# Checks for libraries.

dnl #######################################################################
dnl # Check for glib-2.0 
dnl #######################################################################
PKG_CHECK_MODULES(GLIB, [glib-2.0 gobject-2.0 gmodule-2.0 gthread-2.0],
    [
    ],
    [
        AC_MSG_RESULT([no])
        AC_MSG_ERROR([
You must have the Glib2.0 development headers installed to build.
If you have they installed already you may need to install pkg-config.  
        ])
    ]
)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

dnl #######################################################################
dnl # Check for gtk-2.0 
dnl #######################################################################
PKG_CHECK_MODULES(GTK, [gtk+-2.0 >= 2.0.0],
    [
    ],
    [
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([
You must have the GTK+ 2.0 development headers installed to compile me.
        ])
    ]
)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

dnl We only really need Pango >= 1.4 for decent RTL support
PKG_CHECK_MODULES(PANGO, [pango >= 1.4.0],
            AC_DEFINE(HAVE_PANGO14, 1, [Define if we have Pango 1.4 or newer.]),:)

dnl check for freetype 2
PKG_CHECK_MODULES(PANGOFT2, pangoft2 >= 1.1.0,
	[
		AC_DEFINE(HAVE_PANGOFT2, 1, [Define if we've found pangoft2])
	]
)
AC_SUBST(PANGOFT2_CFLAGS)
AC_SUBST(PANGOFT2_LIBS)

dnl #######################################################################
dnl # Check for libxml 
dnl #######################################################################
PKG_CHECK_MODULES(LIBXML, [libxml-2.0 >= 2.6.0],
    [
    ],
    [
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([
You must have the libxml2 >=2.6.0 development headers installed to build
        ])
    ]
)
AC_SUBST(LIBXML_CFLAGS)
AC_SUBST(LIBXML_LIBS)

dnl #######################################################################
dnl # Check for libcurl 
dnl #######################################################################
PKG_CHECK_MODULES(LIBCURL, [libcurl >= 7],
    [
    ],
    [
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([
You must have the libcurl >= 7 development headers installed to build
        ])
    ]
)
AC_SUBST(LIBCURL_CFLAGS)
AC_SUBST(LIBCURL_LIBS)

dnl #######################################################################
dnl # Check for gstream-1.0 
dnl #######################################################################
dnl
dnl TODO: Depend on gstreamer, and remove the conditional use of
dnl       gst_registry_fork_set_enabled.
AC_ARG_ENABLE(gstreamer,
    [AC_HELP_STRING([--disable-gstreamer], [compile without GStreamer audio support])],
    enable_gst="$enableval", enable_gst="yes") 

if test "x$enable_gst" != "xno"; then
    PKG_CHECK_MODULES(
        GSTREAMER, [gstreamer-0.10 >= 0.10], 
        [
            AC_DEFINE(USE_GSTREAMER, 1, [Use GStreamer for playing sounds])
            AC_SUBST(GSTREAMER_CFLAGS)
            AC_SUBST(GSTREAMER_LIBS)
            AC_CHECK_LIB(gstreamer-0.10, gst_registry_fork_set_enabled,
                [ AC_DEFINE(GST_CAN_DISABLE_FORKING, [],
                [Define if gst_registry_fork_set_enabled exists])],
                [], [$GSTREAMER_LIBS])
        ],
        [
            AC_MSG_RESULT(no)
            AC_MSG_ERROR([GStreamer development headers not found.
                Use --disable-gstreamer if you do not need GStreamer (sound) support.])
        ]
    )
fi


dnl #######################################################################
dnl # Check for gettext 
dnl #######################################################################
dnl AM_GLIB_GNU_GETTEXT defined in aclocal.m4
dnl GLIB_LC_MESSAGES -- ALL_LINGUAGS will be used in this macro function
dnl GLIB_WITH_NLS -- MSGFMT will be used in this macro function
dnl ENABLE_NLS 
ALL_LINGUAS="en_US zh_CN"
AM_GLIB_GNU_GETTEXT

dnl if we don't have msgfmt, then po/ is going to fail -- ensure that
dnl AM_GLIB_GNU_GETTEXT found it
if test x$MSGFMT = "xno" -o x$MSGFMT$GMSGFMT$INTLTOOL_MSGFMT = "x"
then
    AC_ERROR([
The msgfmt command is required to build beasy.  If it is installed
on your system, ensure that it is in your path.  If it is not, install
GNU gettext to continue.

If you have msgfmt installed, but for some reason this error message
is still displayed, you have encountered what appears to be a bug in
third-party configure macros.  Try setting the MSGFMT environment
variable to the absolute path to your msgfmt binary and trying
configure again, like this:

MSGFMT=/path/to/msgfmt ./configure ...
    ])
fi
 
######################################
# Checks for header files.
######################################
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/time.h unistd.h locale.h signal.h stdint.h)

######################################
# Checks for types
######################################
AC_C_CONST

######################################
# Checks for structures
######################################
AC_STRUCT_TM
AC_CHECK_SIZEOF(time_t,,
[
		#include <stdio.h>
		#include <time.h>
])

######################################
# Checks for compiler characteristics.
######################################

######################################
# Checks for library functions.
######################################
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_CHECK_FUNCS(strdup strstr atexit setlocale)

dnl 2. check for inet_aton
AC_CHECK_FUNC(inet_aton, , [AC_CHECK_LIB(resolv, inet_aton, ,
                         [AC_ERROR(inet_aton not found)])])
AC_CHECK_LIB(resolv, __res_query)


dnl 3. check for nsl and socket
AC_CHECK_LIB(nsl, gethostent)
AC_CHECK_FUNC(socket, ,
    [AC_CHECK_LIB(socket, socket, , [AC_ERROR([socket not found])])])

dnl If all goes well, by this point the previous two checks will have
dnl pulled in -lsocket and -lnsl if we need them.
AC_CHECK_FUNC(getaddrinfo,
    [AC_DEFINE([HAVE_GETADDRINFO], [1],
        [Define to 1 if you have the getaddrinfo function.])],
    [AC_CHECK_LIB(socket, getaddrinfo,
        [AC_DEFINE([HAVE_GETADDRINFO]) LIBS="-lsocket -lsnl $LIBS"], , , -lnsl)])

AC_CHECK_FUNCS(inet_ntop)

# Checks for system services

AC_OUTPUT(
 Makefile
 beasy/Makefile
 liboul/Makefile
 plugins/Makefile
)
