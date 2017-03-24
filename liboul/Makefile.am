lib_LTLIBRARIES=liboul.la

liboul_headers= \
	cipher.h \
	circbuffer.h \
	core.h \
	debug.h \
	dnsquery.h \
	dnssrv.h \
	eventloop.h \
	imgstore.h \
	internal.h \
	nat-pmp.h \
	network.h \
	http.h \
	msg.h \
	email.h \
	notify.h \
	ntlm.h \
	plugin.h \
	pluginpref.h \
	prefs.h \
	proxy.h \
	savedstatuses.h \
	sigdef.h \
	signals.h \
	sound.h \
	status.h \
	stun.h \
	upnp.h \
	util.h \
	value.h \
	version.h \
	xmlnode.h 

STATUS_SOURCE_FILES=status.c savedstatuses.c
SIGNAL_SOURCE_FILES=signals.c 
PREF_SOURCE_FILES=prefs.c
PLUGIN_SOURCE_FILES=plugin.c pluginpref.c
NOTIFY_SOURCE_FILES=notify.c msg.c
SOUND_SOURCE_FILES=sound.c
EMAIL_SOURCE_FILES=email.c
NET_SOURCE_FILES=dnsquery.c dnssrv.c nat-pmp.c upnp.c ntlm.c stun.c proxy.c network.c http.c
MISC_SOURCE_FILES=cipher.c circbuffer.c debug.c eventloop.c imgstore.c util.c value.c xmlnode.c

liboul_la_SOURCES=${liboul_headers} \
	version.c \
	core.c \
	$(STATUS_SOURCE_FILES) \
	$(SIGNAL_SOURCE_FILES) \
	$(PREF_SOURCE_FILES) \
	$(PLUGIN_SOURCE_FILES) \
	$(SOUND_SOURCE_FILES) \
	$(EMAIL_SOURCE_FILES) \
	$(NOTIFY_SOURCE_FILES) \
	$(NET_SOURCE_FILES) \
	$(MISC_SOURCE_FILES)

liboul_la_LDFLAGS= -export-dynamic -version-info $(EASY_LT_VERSION_INFO) 

liboul_la_LIBADD= \
    $(GLIB_LIBS) \
    $(GTK_LIBS) \
    $(LIBXML_LIBS)

AM_CPPFLAGS= \
    -DSYSCONFDIR=\"$(sysconfdir)\" \
    -DLIBDIR=\"$(libdir)/beasy/\" \
    $(GLIB_CFLAGS) \
    $(GTK_CFLAGS) \
    $(LIBXML_CFLAGS)

