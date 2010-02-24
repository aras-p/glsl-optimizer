TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = r300

C_SOURCES = \
	r300_blit.c \
	r300_chipset.c \
	r300_context.c \
	r300_debug.c \
	r300_emit.c \
	r300_flush.c \
	r300_fs.c \
	r300_query.c \
	r300_render.c \
	r300_screen.c \
	r300_state.c \
	r300_state_derived.c \
	r300_state_invariant.c \
	r300_vs.c \
	r300_texture.c \
	r300_tgsi_to_rc.c

LIBRARY_INCLUDES = \
	-I$(TOP)/src/mesa/drivers/dri/r300/compiler \
	-I$(TOP)/src/gallium/winsys/drm/radeon/core

COMPILER_ARCHIVE = $(TOP)/src/mesa/drivers/dri/r300/compiler/libr300compiler.a

EXTRA_OBJECTS = \
	$(COMPILER_ARCHIVE)

include ../../Makefile.template

$(COMPILER_ARCHIVE):
	$(MAKE) -C $(TOP)/src/mesa/drivers/dri/r300/compiler
