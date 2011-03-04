TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = nv50

C_SOURCES = \
	nv50_context.c \
	nv50_draw.c \
	nv50_formats.c \
	nv50_miptree.c \
	nv50_resource.c \
	nv50_screen.c \
	nv50_state.c \
	nv50_state_validate.c \
	nv50_surface.c \
	nv50_tex.c \
	nv50_transfer.c \
	nv50_vbo.c \
	nv50_program.c \
	nv50_shader_state.c \
	nv50_pc.c \
	nv50_pc_print.c \
	nv50_pc_emit.c \
	nv50_tgsi_to_nc.c \
	nv50_pc_optimize.c \
	nv50_pc_regalloc.c \
	nv50_push.c \
	nv50_query.c

LIBRARY_INCLUDES = \
	$(LIBDRM_CFLAGS)

include ../../Makefile.template
