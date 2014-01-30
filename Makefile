include ../../../Make.vars 

CONFIGURE_DEPENCIES = $(srcdir)/Makefile.am

EXTRA_DIST = \
	meta.json

pkglib_LTLIBRARIES =		\
	pfswitch13.la	

qpfswitch_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox
qpfswitch_la_SOURCES = pfswitch13.cpp
qpfswitch_la_LDFLAGS = -module -export-dynamic

NOX_RUNTIMEFILES = meta.json

all-local: nox-all-local
clean-local: nox-clean-local 
install-exec-hook: nox-install-local
