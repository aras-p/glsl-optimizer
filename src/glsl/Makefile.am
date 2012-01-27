

# Copyright © 2012 Matt Turner <mattst88@gmail.com>
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

include Makefile.sources

SUBDIRS = glcpp

noinst_LTLIBRARIES = libglslcore.la
lib_LTLIBRARIES = libglsl.la
check_PROGRAMS = glsl_test
noinst_PROGRAMS = builtin_compiler
bin_PROGRAMS = glsl_compiler

AM_CPPFLAGS = \
	$(DEFINES) \
	-I$(top_srcdir)/src/mesa \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/include

BUILT_SOURCES = glsl_parser.h
AM_YFLAGS = -d -p "_mesa_glsl_"
AM_LFLAGS = --nounistd -o$(LEX_OUTPUT_ROOT).c

libglslcore_la_SOURCES = \
	strtod.c \
	glsl_lexer.ll \
	glsl_parser.yy \
	$(LIBGLSL_CXX_FILES)
libglslcore_la_LIBADD = glcpp/libglcpp.la

builtin_compiler_SOURCES = \
	builtin_stubs.cpp \
	main.cpp \
	standalone_scaffolding.cpp \
	../mesa/program/symbol_table.c
builtin_compiler_LDADD = libglslcore.la

libglsl_la_SOURCES = builtin_function.cpp
libglsl_la_LIBADD = libglslcore.la
libglsl_la_LDFLAGS = -avoid-version

glsl_compiler_SOURCES = \
	main.cpp \
	standalone_scaffolding.cpp \
	../mesa/program/symbol_table.c
glsl_compiler_LDADD = libglsl.la

glsl_test_SOURCES = \
	test.cpp \
	test_optpass.cpp \
	standalone_scaffolding.cpp \
	../mesa/program/symbol_table.c
glsl_test_LDADD = libglsl.la

builtin_function.cpp: builtin_compiler
	$(PYTHON2) $(PYTHON_FLAGS) $(top_srcdir)/src/glsl/builtins/tools/generate_builtins.py ./builtin_compiler > builtin_function.cpp || rm -f builtin_function.cpp

if BUILD_SHARED
all-local: libglsl.la
	ln -f .libs/libglsl.so $(top_builddir)/$(LIB_DIR)/libglsl.so;
endif
