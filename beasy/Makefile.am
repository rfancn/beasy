bin_PROGRAMS = beasy

beasy_headers= \
	beasyaux.h \
	beasy.h \
	beasystock.h \
	eggtrayicon.h \
	gtkdialogs.h \
	gtkdocklet.h \
	gtkeventloop.h \
	gtkhtml.h \
	gtkntf_action.h \
	gtkntf_display.h \
	gtkntf_event.h \
	gtkntf_event_info.h \
	gtkntf_file.h \
	gtkntf_gtk_utils.h \
	gtkntf.h \
	gtkntf_item.h \
	gtkntf_item_icon.h \
	gtkntf_item_image.h \
	gtkntf_item_offset.h \
	gtkntf_item_text.h \
	gtkntf_notification.h \
	gtkntf_theme.h \
	gtkntf_theme_info.h \
	gtkntf_theme_ops.h \
	gtkntf_utils.h \
	gtkplugin.h \
	gtkpluginpref.h \
	gtkpref_net.h \
	gtkpref_notify.h \
	gtkpref_email.h \
	gtkpref_plugin.h \
	gtkprefs.h \
	gtkpref_sound.h \
	gtkpref_email.h \
	gtkpref_theme.h \
	gtksound.h \
	gtkemail.h \
	gtkutils.h

beasy_SOURCES=${beasy_headers} \
	gtkeventloop.c \
	beasyaux.c \
	beasystock.c \
	eggtrayicon.c \
	gtkdocklet.c \
	gtkdocklet-x11.c \
	gtksound.c \
	gtkemail.c \
	gtkutils.c \
	gtkplugin.c \
	gtkpluginpref.c \
	gtkdialogs.c \
	gtkhtml.c \
	beasy.c \
	gtkprefs.c \
	gtkpref_net.c \
	gtkpref_sound.c \
	gtkpref_email.c \
	gtkpref_notify.c \
	gtkpref_plugin.c \
	gtkpref_theme.c \
	gtkntf.c \
	gtkntf_action.c \
	gtkntf_display.c \
	gtkntf_event.c \
	gtkntf_event_info.c \
	gtkntf_file.c \
	gtkntf_gtk_utils.c \
	gtkntf_item.c \
	gtkntf_item_icon.c \
	gtkntf_item_image.c \
	gtkntf_item_text.c \
	gtkntf_item_offset.c \
	gtkntf_notification.c \
	gtkntf_theme.c \
	gtkntf_theme_info.c \
	gtkntf_theme_ops.c \
	gtkntf_utils.c

beasy_LDFLAGS = -export-dynamic
beasy_LDADD = \
    @LIBOBJS@ \
    $(GLIB_LIBS) \
    $(LIBXML_LIBS) \
    $(GTK_LIBS) \
    $(GSTREAMER_LIBS) \
	$(PANGOFT2_LIBS) \
    $(top_builddir)/liboul/liboul.la

AM_CPPFLAGS =  \
    -DDATADIR=\"$(datadir)\" \
    -DPKGDATADIR=\"$(pkgdatadir)\" \
    -DLOCALEDIR=\"$(datadir)/locale\" \
    -DSYSCONFDIR=\"$(sysconfdir)\" \
    -I$(top_builddir) \
    -I$(top_builddir)/liboul \
    -I$(top_srcdir) \
    -I$(top_srcdir)/liboul \
    $(GLIB_CFLAGS) \
    $(GSTREAMER_CFLAGS) \
    $(LIBXML_CFLAGS) \
    $(GTK_CFLAGS) \
	$(PANGOFT2_CFLAGS)

