include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	-I$(top_srcdir)/src/gallium/drivers \
	$(GALLIUM_CFLAGS) \
	$(VISIBILITY_CFLAGS)

noinst_LTLIBRARIES = libidentity.la

libidentity_la_SOURCES = $(C_SOURCES)
