## Process this file with automake to produce Makefile.in

SUBDIRS = src

EXTRA_DIST = \
	install-sh \
	ltmain.sh \
	missing \
	compile \
	depcomp


install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/misc; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir); \
	  for miscfile in $(srcdir)/misc/*; do \
	    if test -f $$miscfile; then \
	      $(INSTALL_DATA) $$miscfile $(DESTDIR)$(pkgdatadir); \
	    fi \
	  done \
	fi
	if test -d $(srcdir)/help; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/help; \
	  for helpfile in $(srcdir)/help/*; do \
	    if test -f $$helpfile; then \
	      $(INSTALL_DATA) $$helpfile $(DESTDIR)$(pkgdatadir)/help; \
	    fi \
	  done \
	fi
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
	    fi \
	  done \
	fi
	if test -d $(srcdir)/patches; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/sys-patches; \
	  for patch in $(srcdir)/patches/*; do \
	    if test -f $$patch; then \
	      $(INSTALL_DATA) $$patch $(DESTDIR)$(pkgdatadir)/sys-patches; \
	    fi \
	  done \
	fi
	if test -d $(srcdir)/midimaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/sys-midimaps; \
	  for midimap in $(srcdir)/midimaps/*; do \
	    if test -f $$midimap; then \
	      $(INSTALL_DATA) $$midimap $(DESTDIR)$(pkgdatadir)/sys-midimaps; \
	    fi \
	  done \
	fi
	if test -d $(srcdir)/samples; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/sys-samples; \
	  for sample in $(srcdir)/samples/*; do \
	    if test -f $$sample; then \
	      $(INSTALL_DATA) $$sample $(DESTDIR)$(pkgdatadir)/sys-samples; \
	    fi \
	  done \
	fi
	if test -d $(srcdir)/themes; then \
	  for theme in $(srcdir)/themes/Phasex-*; do \
	    $(mkinstalldirs) $(DESTDIR)$(datadir)/$$theme; \
	    $(mkinstalldirs) $(DESTDIR)$(datadir)/$$theme/gtk-2.0; \
	    if test -f $$theme; then \
	      $(INSTALL_DATA) $$theme $(DESTDIR)$(datadir)/$$theme/gtk-2.0/gtkrc; \
	    fi \
	  done \
	fi


dist-hook:
	if test -d misc; then \
	  mkdir $(distdir); \
	  for miscfile in misc/*; do \
	    if test -f $$miscfile; then \
	      cp -p $$miscfile $(distdir); \
	    fi \
	  done \
	fi
	if test -d help; then \
	  mkdir $(distdir)/help; \
	  for helpfile in help/*; do \
	    if test -f $$helpfile; then \
	      cp -p $$helpfile $(distdir)/help; \
	    fi \
	  done \
	fi
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi
	if test -d patches; then \
	  mkdir $(distdir)/sys-patches; \
	  for patch in patches/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$patch $(distdir)/sys-patches; \
	    fi \
	  done \
	fi
	if test -d midimaps; then \
	  mkdir $(distdir)/sys-midimaps; \
	  for midimap in midimaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$midimap $(distdir)/sys-midimaps; \
	    fi \
	  done \
	fi
	if test -d samples; then \
	  mkdir $(distdir)/sys-samples; \
	  for sample in samples/*; do \
	    if test -f $$sample; then \
	      cp -p $$sample $(distdir)/sys-samples; \
	    fi \
	  done \
	fi
	if test -d themes; then \
	  for theme in themes/Phasex-*; do \
	    mkdir -p $(datadir)/themes/$$theme/gtk-2.0; \
	    if test -f $$theme; then \
	      cp -p $$theme $(datadir)/themes/$$theme/gtk-2.0/gtkrc; \
	    fi \
	  done \
	fi
#	if test -f phasex.desktop; then \
#	  mkdir -p $(distdir); \
#	  cp -p phasex.desktop $(distdir); \
#	fi


distclean-local:
	rm -f *~ doc/*~ help/*~ midimaps/*~ misc/*~ patches/*~ themes/*~
	rm -rf autom4te.cache
