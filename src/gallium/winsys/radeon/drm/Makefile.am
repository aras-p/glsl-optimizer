include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	-I$(top_srcdir)/include \
	$(GALLIUM_CFLAGS) \
	$(RADEON_CFLAGS)

noinst_LTLIBRARIES = libradeonwinsys.la

libradeonwinsys_la_SOURCES = $(C_SOURCES)

#XXX: Delete this when all targets using the radeon winsys are converted to automake.
all-local: libradeonwinsys.la
	ln -f $(builddir)/.libs/libradeonwinsys.a $(builddir)/libradeonwinsys.a
