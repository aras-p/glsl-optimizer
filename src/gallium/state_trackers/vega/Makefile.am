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

AM_CFLAGS = $(GALLIUM_CFLAGS)
AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/mapi \
	-DVEGA_VERSION_STRING=\"$(MESA_VERSION)\"

noinst_LTLIBRARIES = libvega.la

libvega_la_SOURCES =	\
	api.c		\
	api_context.c	\
	api_filters.c	\
	api_images.c	\
	api_masks.c	\
	api_misc.c	\
	api_paint.c	\
	api_params.c	\
	api_path.c	\
	api_text.c	\
	api_transform.c	\
	arc.c		\
	bezier.c	\
	handle.c	\
	image.c		\
	mask.c		\
	paint.c		\
	path.c		\
	polygon.c	\
	renderer.c	\
	shader.c	\
	shaders_cache.c	\
	stroker.c	\
	text.c		\
	vg_context.c	\
	vg_manager.c	\
	vg_state.c	\
	vg_translate.c	\
	vgu.c

BUILT_SOURCES = api_tmp.h
CLEANFILES = api_tmp.h

api_tmp.h: $(top_srcdir)/src/mapi/vgapi/vgapi.csv $(top_srcdir)/src/mapi/mapi/mapi_abi.py
	$(AM_V_GEN) $(PYTHON2) $(PYTHON_FLAGS) $(top_srcdir)/src/mapi/mapi/mapi_abi.py \
			--printer vgapi --mode app $< > $@
