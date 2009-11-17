TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = svga

C_SOURCES = \
	svgadump/st_shader_dump.c \
	svgadump/st_shader_op.c \
	svgadump/svga_dump.c \
	svga_cmd.c \
	svga_context.c \
	svga_draw.c \
	svga_draw_arrays.c \
	svga_draw_elements.c \
	svga_pipe_blend.c \
	svga_pipe_blit.c \
	svga_pipe_clear.c \
	svga_pipe_constants.c \
	svga_pipe_depthstencil.c \
	svga_pipe_draw.c \
	svga_pipe_flush.c \
	svga_pipe_fs.c \
	svga_pipe_misc.c \
	svga_pipe_query.c \
	svga_pipe_rasterizer.c \
	svga_pipe_sampler.c \
	svga_pipe_vertex.c \
	svga_pipe_vs.c \
	svga_screen.c \
	svga_screen_buffer.c \
	svga_screen_texture.c \
	svga_screen_cache.c \
	svga_state.c \
	svga_state_need_swtnl.c \
	svga_state_constants.c \
	svga_state_framebuffer.c \
	svga_state_rss.c \
	svga_state_tss.c \
	svga_state_vdecl.c \
	svga_state_fs.c \
	svga_state_vs.c \
	svga_swtnl_backend.c \
	svga_swtnl_draw.c \
	svga_swtnl_state.c \
	svga_tgsi.c \
	svga_tgsi_decl_sm20.c \
	svga_tgsi_decl_sm30.c \
	svga_tgsi_insn.c

LIBRARY_INCLUDES = \
	-I$(TOP)/src/gallium/drivers/svga/include

LIBRARY_DEFINES = \
	-DHAVE_STDINT_H -DHAVE_SYS_TYPES_H

CC = gcc -fvisibility=hidden -msse -msse2

# Set the gnu99 standard to enable anonymous structs in vmware headers.
#
CFLAGS = -Wall -Wmissing-prototypes -std=gnu99 -ffast-math \
	$(OPT_FLAGS) $(PIC_FLAGS) $(ARCH_FLAGS) $(DEFINES) $(ASM_FLAGS)

include ../../Makefile.template
