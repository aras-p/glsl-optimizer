TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = nvfx

C_SOURCES = \
	nv04_2d.c \
	nvfx_buffer.c \
	nvfx_context.c \
	nvfx_clear.c \
	nvfx_draw.c \
	nvfx_fragprog.c \
	nvfx_fragtex.c \
	nv30_fragtex.c \
	nv40_fragtex.c \
	nvfx_miptree.c \
	nvfx_push.c \
	nvfx_query.c \
	nvfx_resource.c \
	nvfx_screen.c \
	nvfx_state.c \
	nvfx_state_emit.c \
	nvfx_state_fb.c \
	nvfx_surface.c \
	nvfx_transfer.c \
	nvfx_vbo.c \
	nvfx_vertprog.c

LIBRARY_INCLUDES = \
	$(LIBDRM_CFLAGS) \
	-I$(TOP)/src/gallium/drivers/nouveau/include

include ../../Makefile.template
