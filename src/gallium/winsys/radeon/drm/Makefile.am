include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	$(GALLIUM_WINSYS_CFLAGS) \
	$(RADEON_CFLAGS)

noinst_LTLIBRARIES = libradeonwinsys.la

libradeonwinsys_la_SOURCES = $(C_SOURCES)

EXTRA_DIST = $(TOOLS_HDR)
