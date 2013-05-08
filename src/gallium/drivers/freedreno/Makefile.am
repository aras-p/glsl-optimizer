include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

noinst_LTLIBRARIES = libfreedreno.la

AM_CFLAGS = \
	-Wno-packed-bitfield-compat \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/gallium/drivers/freedreno/a3xx \
	-I$(top_srcdir)/src/gallium/drivers/freedreno/a2xx \
	$(GALLIUM_CFLAGS) \
	$(FREEDRENO_CFLAGS) \
	$(VISIBILITY_CFLAGS)

SUBDIRS = a2xx a3xx

libfreedreno_la_SOURCES = $(C_SOURCES)

libfreedreno_la_LIBADD = \
	a3xx/libfd3xx.la \
	a2xx/libfd2xx.la

