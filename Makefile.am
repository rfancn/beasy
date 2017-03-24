SUBDIRS=liboul beasy plugins

EXTRA_DIST= \
	regen.sh \
	run.sh \
	debug.sh \
	build_rpm.sh \
	beasy.spec \
	build_deb.sh

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/icons; then \
      $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/icons; \
      cp -fr  $(srcdir)/icons/* $(DESTDIR)$(pkgdatadir)/icons; \
    fi
	if test -d $(srcdir)/pixmaps; then \
      $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
      cp -fr  $(srcdir)/pixmaps/* $(DESTDIR)$(pkgdatadir)/pixmaps; \
    fi
	if test -d $(srcdir)/sounds; then \
      $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/sounds; \
      cp -fr  $(srcdir)/sounds/* $(DESTDIR)$(pkgdatadir)/sounds; \
    fi
	if test -d $(srcdir)/themes; then \
      $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/themes; \
      cp -fr  $(srcdir)/themes/* $(DESTDIR)$(pkgdatadir)/themes; \
    fi

dist-hook:
	if test -d $(srcdir)/icons; then \
		mkdir $(distdir)/icons; \
		for icon in $(srcdir)/icons/*; do \
			cp -fr $$icon $(distdir)/icons; \
		done \
	fi
	if test -d $(srcdir)/pixmaps; then \
		mkdir $(distdir)/pixmaps; \
		for pixmap in $(srcdir)/pixmaps/*; do \
			cp -fr $$pixmap $(distdir)/pixmaps; \
		done \
	fi
	if test -d $(srcdir)/sounds; then \
		mkdir $(distdir)/sounds; \
		for sound in $(srcdir)/sounds/*; do \
			cp -fr $$sound $(distdir)/sounds; \
		done \
	fi
	if test -d $(srcdir)/themes; then \
		mkdir $(distdir)/themes; \
		for theme in $(srcdir)/themes/*; do \
			cp -fr $$theme $(distdir)/themes; \
		done \
	fi

