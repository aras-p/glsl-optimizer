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
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

SUBDIRS = xmlpool

include Makefile.sources

AM_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/ \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/src/mesa/ \
	$(DEFINES) \
	$(EXPAT_CFLAGS) \
	$(VISIBILITY_CFLAGS)

noinst_LTLIBRARIES = \
	libdricommon.la \
	libmegadriver_stub.la \
	libdri_test_stubs.la

libdricommon_la_SOURCES = $(DRI_COMMON_FILES)

libdri_test_stubs_la_SOURCES = $(test_stubs_FILES)
libdri_test_stubs_la_CFLAGS = $(AM_CFLAGS) -DNO_MAIN

libmegadriver_stub_la_SOURCES = $(megadriver_stub_FILES)

sysconf_DATA = drirc

if DRICOMMON_NEED_LIBDRM
AM_CFLAGS += $(LIBDRM_CFLAGS)
libdricommon_la_LIBADD = $(LIBDRM_LIBS)
else
AM_CFLAGS += -D__NOT_HAVE_DRM_H
endif
