include $(top_srcdir)/src/gallium/Automake.inc

noinst_LTLIBRARIES = libfd3xx.la

AM_CFLAGS = \
	-Wno-packed-bitfield-compat \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/gallium/drivers/freedreno \
	$(GALLIUM_CFLAGS) \
	$(FREEDRENO_CFLAGS) \
	$(VISIBILITY_CFLAGS)

libfd3xx_la_SOURCES = \
	fd3_blend.c \
	fd3_compiler.c \
	fd3_context.c \
	fd3_draw.c \
	fd3_emit.c \
	fd3_gmem.c \
	fd3_program.c \
	fd3_rasterizer.c \
	fd3_screen.c \
	fd3_texture.c \
	fd3_util.c \
	fd3_zsa.c \
	disasm-a3xx.c \
	ir-a3xx.c
