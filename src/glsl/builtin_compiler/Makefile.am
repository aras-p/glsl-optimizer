# Copyright © 2012 Jon TURNEY
# Copyright © 2012 Thierry Reding
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

CC = @CC_FOR_BUILD@
CFLAGS = @CFLAGS_FOR_BUILD@
CPP = @CPP_FOR_BUILD@
CPPFLAGS = @CPPFLAGS_FOR_BUILD@
CXX = @CXX_FOR_BUILD@
CXXFLAGS = @CXXFLAGS_FOR_BUILD@
LD = @LD_FOR_BUILD@
LDFLAGS = @LDFLAGS_FOR_BUILD@

AM_CFLAGS = \
	-I $(top_srcdir)/include \
	-I $(top_srcdir)/src/mapi \
	-I $(top_srcdir)/src/mesa \
	-I $(GLSL_SRCDIR) \
	-I $(GLSL_SRCDIR)/glcpp \
	-I $(GLSL_BUILDDIR) \
	$(DEFINES_FOR_BUILD)

AM_CXXFLAGS = $(AM_CFLAGS)

include ../Makefile.sources

noinst_PROGRAMS = builtin_compiler
noinst_LTLIBRARIES = libglslcore.la libglcpp.la

libglcpp_la_SOURCES =					\
	$(LIBGLCPP_GENERATED_FILES)			\
	$(LIBGLCPP_FILES)

libglslcore_la_SOURCES =				\
	$(BUILTIN_COMPILER_GENERATED_CXX_FILES)		\
	$(LIBGLSL_FILES)

builtin_compiler_SOURCES = \
	$(top_srcdir)/src/mesa/main/hash_table.c	\
	$(top_srcdir)/src/mesa/main/imports.c		\
	$(top_srcdir)/src/mesa/program/prog_hash_table.c\
	$(top_srcdir)/src/mesa/program/symbol_table.c	\
	$(BUILTIN_COMPILER_CXX_FILES)			\
	$(GLSL_COMPILER_CXX_FILES)
builtin_compiler_LDADD = libglslcore.la libglcpp.la
