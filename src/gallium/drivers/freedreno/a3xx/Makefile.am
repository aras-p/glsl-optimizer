include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

noinst_LTLIBRARIES = libfd3xx.la

AM_CFLAGS = \
	-Wno-packed-bitfield-compat \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/gallium/drivers/freedreno \
	$(GALLIUM_CFLAGS) \
	$(FREEDRENO_CFLAGS) \
	$(VISIBILITY_CFLAGS)

libfd3xx_la_SOURCES = $(C_SOURCES)
