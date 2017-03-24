plugindir=$(libdir)/beasy

plugin_LTLIBRARIES= example.la qmon.la

example_la_LDFLAGS = -module -avoid-version
example_la_SOURCES = example/example.c

qmon_la_LDFLAGS = -module -avoid-version
qmon_la_SOURCES = \
	qmon/qmon.c \
	qmon/qmon.h \
	qmon/qmonpref.c \
	qmon/qmonpref.h \
	qmon/qmonxml.c \
	qmon/qmonxml.h \
	qmon/qmonsession.c \
	qmon/qmonsession.h \
	qmon/qmonoptions.c \
	qmon/qmonoptions.h \
	qmon/qmonreport.c \
	qmon/qmonreport.h \
	qmon/qmonplugin.c \
	qmon/qmonplugin.h

qmon_la_LIBADD = $(GTK_LIBS) $(LIBXML_LIBS) $(LIBCURL_LIBS) 

AM_CPPFLAGS = \
    -DDATADIR=\"$(datadir)\" \
    -I$(top_srcdir)/beasy \
    -I$(top_builddir)/beasy \
    -I$(top_srcdir)/liboul \
    -I$(top_builddir)/liboul \
    $(DEBUG_CFLAGS) \
    $(LIBXML_CFLAGS) \
    $(LIBCURL_CFLAGS) \
    $(GLIB_CFLAGS) \
    $(GTK_CFLAGS) \
    $(PLUGIN_CFLAGS)
