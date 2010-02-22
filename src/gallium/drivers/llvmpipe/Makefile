TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = llvmpipe

DEFINES += -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS

C_SOURCES = \
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
	lp_tile_image.c \
	lp_tile_soa.c

CPP_SOURCES = \


include ../../Makefile.template

lp_tile_soa.c: lp_tile_soa.py ../../auxiliary/util/u_format_parse.py ../../auxiliary/util/u_format_access.py ../../auxiliary/util/u_format.csv
	python lp_tile_soa.py ../../auxiliary/util/u_format.csv > $@


testprogs := lp_test_format	\
	     lp_test_blend	\
	     lp_test_conv

LIBS += $(GL_LIB_DEPS) -L. -lllvmpipe -L../../auxiliary/ -lgallium

$(testprogs): lp_test_% : lp_test_%.o lp_test_main.o libllvmpipe.a
	$(LD) $(filter %.o,$^) -o $@ -Wl,--start-group  $(LIBS) -Wl,--end-group

default: $(testprogs)
