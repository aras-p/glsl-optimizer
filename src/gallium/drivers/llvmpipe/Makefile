TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = llvmpipe

C_SOURCES = \
	lp_bld_alpha.c \
	lp_bld_blend.c \
	lp_bld_blend_aos.c \
	lp_bld_blend_logicop.c \
	lp_bld_blend_soa.c \
	lp_bld_depth.c \
	lp_bld_interp.c \
	lp_clear.c \
	lp_context.c \
	lp_draw_arrays.c \
	lp_fence.c \
	lp_flush.c \
	lp_jit.c \
	lp_memory.c \
	lp_perf.c \
	lp_query.c \
	lp_rast.c \
	lp_rast_debug.c \
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
	lp_state_setup.c \
	lp_state_gs.c \
	lp_state_rasterizer.c \
	lp_state_sampler.c \
        lp_state_so.c \
	lp_state_surface.c \
	lp_state_vertex.c \
	lp_state_vs.c \
	lp_surface.c \
	lp_tex_sample.c \
	lp_texture.c \
	lp_tile_image.c \
	lp_tile_soa.c

CPP_SOURCES = \

PROGS := lp_test_format	\
	 lp_test_arit	\
	 lp_test_blend	\
	 lp_test_conv	\
	 lp_test_printf

# Need this for the lp_test_*.o files
CLEAN_EXTRA = *.o

include ../../Makefile.template

PROGS_DEPS := ../../auxiliary/libgallium.a

lp_tile_soa.c: lp_tile_soa.py ../../auxiliary/util/u_format_parse.py ../../auxiliary/util/u_format_pack.py ../../auxiliary/util/u_format.csv
	$(PYTHON2) $(PYTHON_FLAGS) lp_tile_soa.py ../../auxiliary/util/u_format.csv > $@

LDFLAGS += $(LLVM_LDFLAGS)
LIBS += -L../../auxiliary/ -lgallium libllvmpipe.a $(LLVM_LIBS) $(GL_LIB_DEPS)
LD=$(CXX)

$(PROGS): lp_test_main.o libllvmpipe.a

