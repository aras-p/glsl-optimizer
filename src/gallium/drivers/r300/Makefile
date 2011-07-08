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
	r300_hyperz.c \
	r300_query.c \
	r300_render.c \
	r300_render_stencilref.c \
	r300_render_translate.c \
	r300_resource.c \
	r300_screen.c \
	r300_screen_buffer.c \
	r300_state.c \
	r300_state_derived.c \
	r300_vs.c \
	r300_vs_draw.c \
	r300_texture.c \
	r300_texture_desc.c \
	r300_tgsi_to_rc.c \
	r300_transfer.c

LIBRARY_INCLUDES = \
	-I$(TOP)/src/mesa/drivers/dri/r300/compiler \
	-I$(TOP)/include

COMPILER_ARCHIVE = $(TOP)/src/mesa/drivers/dri/r300/compiler/libr300compiler.a

EXTRA_OBJECTS = \
	$(COMPILER_ARCHIVE)

include ../../Makefile.template

.PHONY: $(COMPILER_ARCHIVE)
$(COMPILER_ARCHIVE):
	$(MAKE) -C $(TOP)/src/mesa/drivers/dri/r300/compiler
