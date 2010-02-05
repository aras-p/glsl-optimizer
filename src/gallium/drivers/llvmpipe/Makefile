TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = llvmpipe

DEFINES += -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS

C_SOURCES = \
	lp_bld_alpha.c \
	lp_bld_arit.c \
	lp_bld_blend_aos.c \
	lp_bld_blend_logicop.c \
	lp_bld_blend_soa.c \
	lp_bld_const.c \
	lp_bld_conv.c \
	lp_bld_debug.c \
	lp_bld_depth.c \
	lp_bld_flow.c \
	lp_bld_format_aos.c \
	lp_bld_format_query.c \
	lp_bld_format_soa.c \
	lp_bld_interp.c \
	lp_bld_intr.c \
	lp_bld_logic.c \
	lp_bld_pack.c \
	lp_bld_sample.c \
	lp_bld_sample_soa.c \
	lp_bld_swizzle.c \
	lp_bld_struct.c \
	lp_bld_tgsi_soa.c \
	lp_bld_type.c \
	lp_buffer.c \
	lp_clear.c \
	lp_context.c \
	lp_draw_arrays.c \
	lp_fence.c \
	lp_flush.c \
	lp_jit.c \
	lp_perf.c \
	lp_query.c \
	lp_rast.c \
	lp_rast_tri.c \
	lp_scene.c \
	lp_scene_queue.c \
	lp_screen.c \
	lp_setup.c \
	lp_setup_line.c \
	lp_setup_point.c \
	lp_setup_tri.c \
	lp_setup_vbuf.c \
	lp_state_blend.c \
	lp_state_clip.c \
	lp_state_derived.c \
	lp_state_fs.c \
	lp_state_rasterizer.c \
	lp_state_sampler.c \
	lp_state_surface.c \
	lp_state_vertex.c \
	lp_state_vs.c \
	lp_surface.c \
	lp_tex_sample_llvm.c \
	lp_texture.c \
	lp_tile_soa.c

CPP_SOURCES = \
	lp_bld_misc.cpp

include ../../Makefile.template

lp_tile_soa.c: lp_tile_soa.py ../../auxiliary/util/u_format_parse.py ../../auxiliary/util/u_format_access.py ../../auxiliary/util/u_format.csv
	python lp_tile_soa.py ../../auxiliary/util/u_format.csv > $@


# to make a .s file to inspect assembly code
.c.s:
	$(CC) -S $(INCLUDES) $(DEFINES) $(CFLAGS) $(LIBRARY_DEFINES) $<


testprogs := lp_test_format	\
	     lp_test_blend	\
	     lp_test_conv

LIBS += $(GL_LIB_DEPS) -L. -lllvmpipe -L../../auxiliary/ -lgallium

$(testprogs): lp_test_% : lp_test_%.o lp_test_main.o
	$(LD) $^ -o $@ -Wl,--start-group  $(LIBS) -Wl,--end-group


default: $(testprogs)
