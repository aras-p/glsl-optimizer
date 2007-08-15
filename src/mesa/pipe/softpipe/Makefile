
TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = softpipe

DRIVER_SOURCES = \
	sp_clear.c \
	sp_flush.c \
	sp_context.c \
	sp_prim_setup.c \
	sp_quad.c \
	sp_quad_alpha_test.c \
	sp_quad_blend.c \
	sp_quad_bufloop.c \
	sp_quad_colormask.c \
	sp_quad_coverage.c \
	sp_quad_depth_test.c \
	sp_quad_fs.c \
	sp_quad_occlusion.c \
	sp_quad_output.c \
	sp_quad_stencil.c \
	sp_quad_stipple.c \
	sp_region.c \
	sp_state_blend.c \
	sp_state_clip.c \
	sp_state_derived.c \
	sp_state_fs.c \
	sp_state_sampler.c \
	sp_state_setup.c \
	sp_state_surface.c \
	sp_state_vertex.c \
	sp_tex_layout.c \
	sp_tex_sample.c \
	sp_surface.c 

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

include ../Makefile.template

symlinks:

