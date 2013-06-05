################################################################################

# Meta-driver which combines whichever software rasterizers have been
# built into a single convenience library.

include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	-I$(top_srcdir)/src/gallium/drivers \
	$(GALLIUM_CFLAGS) \
	$(VISIBILITY_CFLAGS)

noinst_LTLIBRARIES = libgalahad.la

libgalahad_la_SOURCES = $(C_SOURCES)
