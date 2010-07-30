## Process this file with automake to produce Makefile.in

EXTRA_DIST = phasex.spec.in

bin_PROGRAMS = phasex

phasex_SOURCES = \
	phasex.c phasex.h config.h \
	engine.c engine.h \
	filter.c filter.h \
	wave.c wave.h \
	jack.c jack.h \
	midi.c midi.h \
	patch.c patch.h \
	param.c param.h \
	callback.c callback.h \
	gtkui.c gtkui.h \
	bank.c bank.h \
	settings.c settings.h \
	help.c help.h \
	gtkknob.c gtkknob.h \
	lash.c lash.h

AM_CFLAGS   = @PHASEX_CFLAGS@
AM_CPPFLAGS = $(EXTRA_CPPFLAGS) @PHASEX_CPPFLAGS@
AM_LDFLAGS  = $(INTLLIBS) @PHASEX_LIBS@

distclean-local:
	rm -f *~
