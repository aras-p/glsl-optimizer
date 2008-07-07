TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = nv04

DRIVER_SOURCES = \
	nv04_clear.c \
	nv04_context.c \
	nv04_fragprog.c \
	nv04_fragtex.c \
	nv04_miptree.c \
	nv04_prim_vbuf.c \
	nv04_screen.c \
	nv04_state.c \
	nv04_state_emit.c \
	nv04_surface.c \
	nv04_vbo.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

include ../../Makefile.template

symlinks:

