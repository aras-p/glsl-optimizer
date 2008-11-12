TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = nv20

DRIVER_SOURCES = \
	nv20_clear.c \
	nv20_context.c \
	nv20_fragprog.c \
	nv20_fragtex.c \
	nv20_miptree.c \
	nv20_prim_vbuf.c \
	nv20_screen.c \
	nv20_state.c \
	nv20_state_emit.c \
	nv20_surface.c \
	nv20_vbo.c
#	nv20_vertprog.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

include ../../Makefile.template

symlinks:

