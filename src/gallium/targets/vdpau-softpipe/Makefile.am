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

AM_CFLAGS = \
	$(GALLIUM_CFLAGS) \
	$(PTHREAD_CFLAGS) \
	$(LIBDRM_CFLAGS)
AM_CPPFLAGS = \
	-DGALLIUM_SOFTPIPE \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/gallium/winsys

vdpaudir = $(VDPAU_LIB_INSTALL_DIR)
vdpau_LTLIBRARIES = libvdpau_softpipe.la

libvdpau_softpipe_la_SOURCES = \
	$(top_srcdir)/src/gallium/auxiliary/vl/vl_winsys_dri.c

libvdpau_softpipe_la_LDFLAGS = \
	-module \
	-version-number $(VDPAU_MAJOR):$(VDPAU_MINOR) \
	-shared \
	-no-undefined

libvdpau_softpipe_la_LIBADD = \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/gallium/state_trackers/vdpau/libvdpautracker.la \
	$(top_builddir)/src/gallium/winsys/sw/xlib/libws_xlib.la \
	$(top_builddir)/src/gallium/drivers/softpipe/libsoftpipe.la \
	$(VDPAU_LIBS) \
	$(DLOPEN_LIBS) \
	$(LIBDRM_LIBS)

if HAVE_MESA_LLVM
libvdpau_softpipe_la_LINK = $(CXXLINK) $(libvdpau_softpipe_la_LDFLAGS)
# Mention a dummy pure C++ file to trigger generation of the $(LINK) variable
nodist_EXTRA_libvdpau_softpipe_la_SOURCES = dummy-cpp.cpp

AM_CPPFLAGS += -DGALLIUM_LLVMPIPE
libvdpau_softpipe_la_LIBADD += $(top_builddir)/src/gallium/drivers/llvmpipe/libllvmpipe.la $(LLVM_LIBS)
else
libvdpau_softpipe_la_LINK = $(LINK) $(libvdpau_softpipe_la_LDFLAGS)
# Mention a dummy pure C file to trigger generation of the $(LINK) variable
nodist_EXTRA_libvdpau_softpipe_la_SOURCES = dummy-c.c
endif

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.
all-local: libvdpau_softpipe.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR)/gallium
	ln -f .libs/libvdpau_softpipe.so* $(top_builddir)/$(LIB_DIR)/gallium/
