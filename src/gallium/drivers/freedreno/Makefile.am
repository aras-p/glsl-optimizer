include $(top_srcdir)/src/gallium/Automake.inc

noinst_LTLIBRARIES = libfreedreno.la

AM_CFLAGS = \
	-Wno-packed-bitfield-compat \
	-I$(top_srcdir)/src/gallium/drivers \
	$(GALLIUM_CFLAGS) \
	$(FREEDRENO_CFLAGS) \
	$(PIC_FLAGS) \
	$(VISIBILITY_CFLAGS)

libfreedreno_la_SOURCES = \
	freedreno_util.c \
	freedreno_fence.c \
	freedreno_resource.c \
	freedreno_surface.c \
	freedreno_vbo.c \
	freedreno_blend.c \
	freedreno_rasterizer.c \
	freedreno_zsa.c \
	freedreno_state.c \
	freedreno_clear.c \
	freedreno_program.c \
	freedreno_texture.c \
	freedreno_context.c \
	freedreno_screen.c \
	freedreno_gmem.c \
	freedreno_compiler.c \
	ir-a2xx.c \
	disasm.c

