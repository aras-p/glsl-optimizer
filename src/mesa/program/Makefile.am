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

include ../sources.mak

AM_CPPFLAGS = $(API_DEFINES) $(DEFINES) $(INCLUDE_DIRS)
AM_CFLAGS = $(VISIBILITY_CFLAGS)
AM_CXXFLAGS = $(VISIBILITY_CXXFLAGS)
libdricore_program_la_CFLAGS = $(NOVISIBILITY_CFLAGS)
libdricore_program_la_CXXFLAGS = $(NOVISIBILITY_CXXFLAGS)

SRCDIR = $(top_srcdir)/src/mesa/
BUILDDIR = $(top_builddir)/src/mesa/

if NEED_LIBDRICORE
DRICORE_LIB = libdricore_program.la
endif

noinst_LTLIBRARIES = $(DRICORE_LIB)
if NEED_LIBPROGRAM
noinst_LTLIBRARIES += libprogram.la
else
check_LTLIBRARIES = libprogram.la
endif

libprogram_la_SOURCES = $(PROGRAM_FILES)
libdricore_program_la_SOURCES = $(PROGRAM_FILES)

lex.yy.c: program_lexer.l
	$(AM_V_GEN) $(LEX) --never-interactive --outfile=$@ $<

program_parse.tab.c program_parse.tab.h: program_parse.y
	$(AM_V_GEN) $(YACC) -p "_mesa_program_" -v -d --output=program_parse.tab.c $<

BUILT_SOURCES = \
	program_parse.tab.c \
	program_parse.tab.h \
	lex.yy.c
CLEANFILES = $(BUILT_SOURCES)
