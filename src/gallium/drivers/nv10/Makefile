TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = nv10

DRIVER_SOURCES = \
	nv10_clear.c \
	nv10_context.c \
	nv10_fragprog.c \
	nv10_fragtex.c \
	nv10_miptree.c \
	nv10_prim_vbuf.c \
	nv10_screen.c \
	nv10_state.c \
	nv10_state_emit.c \
	nv10_surface.c \
	nv10_vbo.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

include ../../Makefile.template

symlinks:

