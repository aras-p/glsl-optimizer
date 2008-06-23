TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = nv30

DRIVER_SOURCES = \
	nv30_clear.c \
	nv30_context.c \
	nv30_draw.c \
	nv30_fragprog.c \
	nv30_fragtex.c \
	nv30_miptree.c \
	nv30_query.c \
	nv30_screen.c \
	nv30_state.c \
	nv30_state_blend.c \
	nv30_state_emit.c \
	nv30_state_fb.c \
	nv30_state_rasterizer.c \
	nv30_state_scissor.c \
	nv30_surface.c \
	nv30_vbo.c \
	nv30_vertprog.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

include ../../Makefile.template

symlinks:

