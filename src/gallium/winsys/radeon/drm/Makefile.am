include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	-I$(top_srcdir)/include \
	$(GALLIUM_CFLAGS) \
	$(RADEON_CFLAGS)

noinst_LTLIBRARIES = libradeonwinsys.la

libradeonwinsys_la_SOURCES = $(C_SOURCES)
