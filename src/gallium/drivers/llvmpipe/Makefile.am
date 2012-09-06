# Copyright © 2012 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

include $(top_srcdir)/src/gallium/Automake.inc

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/include \
	$(GALLIUM_CFLAGS)
AM_CFLAGS = $(LLVM_CFLAGS)
AM_CXXFLAGS= $(LLVM_CXXFLAGS)

noinst_LTLIBRARIES = libllvmpipe.la

libllvmpipe_la_SOURCES = \
	lp_bld_alpha.c \
	lp_bld_blend.c \
	lp_bld_blend_aos.c \
	lp_bld_blend_logicop.c \
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
	lp_tile_image.c

libllvmpipe_la_LDFLAGS = $(LLVM_LDFLAGS)

check_PROGRAMS = \
	lp_test_format	\
	lp_test_arit	\
	lp_test_blend	\
	lp_test_conv	\
	lp_test_printf
TESTS = $(check_PROGRAMS)

lp_test_format_SOURCES = lp_test_format.c lp_test_main.c
lp_test_format_LDADD = libllvmpipe.la ../../auxiliary/libgallium.la $(LLVM_LIBS)
nodist_EXTRA_lp_test_format_SOURCES = dummy.cpp

lp_test_arit_SOURCES = lp_test_arit.c lp_test_main.c
lp_test_arit_LDADD = libllvmpipe.la ../../auxiliary/libgallium.la $(LLVM_LIBS)
nodist_EXTRA_lp_test_arit_SOURCES = dummy.cpp

lp_test_blend_SOURCES = lp_test_blend.c lp_test_main.c
lp_test_blend_LDADD = libllvmpipe.la ../../auxiliary/libgallium.la $(LLVM_LIBS)
nodist_EXTRA_lp_test_blend_SOURCES = dummy.cpp

lp_test_conv_SOURCES = lp_test_conv.c lp_test_main.c
lp_test_conv_LDADD = libllvmpipe.la ../../auxiliary/libgallium.la $(LLVM_LIBS)
nodist_EXTRA_lp_test_conv_SOURCES = dummy.cpp

lp_test_printf_SOURCES = lp_test_printf.c lp_test_main.c
lp_test_printf_LDADD = libllvmpipe.la ../../auxiliary/libgallium.la $(LLVM_LIBS)
nodist_EXTRA_lp_test_printf_SOURCES = dummy.cpp

