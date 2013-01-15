# Copyright © 2012 Jon TURNEY
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

if CROSS_COMPILING
SUBDIRS = builtin_compiler glcpp
else
SUBDIRS = glcpp
endif

AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/src/mesa/ \
	$(DEFINES) \
	$(API_DEFINES)
AM_CFLAGS = $(VISIBILITY_CFLAGS)
AM_CXXFLAGS = $(VISIBILITY_CXXFLAGS)

AM_LFLAGS = --nounistd -o$(LEX_OUTPUT_ROOT).c

include Makefile.sources

noinst_LTLIBRARIES = libglsl.la
check_PROGRAMS = glsl_test

libglsl_la_SOURCES = \
	glsl_lexer.ll \
	glsl_parser.cc \
	$(LIBGLSL_FILES) \
	builtin_function.cpp

libglsl_la_LIBADD = glcpp/libglcpp.la
libglsl_la_LDFLAGS =

glsl_test_SOURCES = \
	$(top_srcdir)/src/mesa/main/hash_table.c \
	$(top_srcdir)/src/mesa/program/prog_hash_table.c \
	$(top_srcdir)/src/mesa/program/symbol_table.c \
	$(GLSL_SRCDIR)/standalone_scaffolding.cpp \
	test.cpp \
	test_optpass.cpp

glsl_test_LDADD = libglsl.la

if CROSS_COMPILING
BUILTIN_COMPILER = builtin_compiler/builtin_compiler$(BUILD_EXEEXT)
else
BUILTIN_COMPILER = builtincompiler$(BUILD_EXEEXT)

noinst_PROGRAMS = builtincompiler
noinst_LTLIBRARIES += libglslcore.la

libglslcore_la_SOURCES =				\
	glsl_lexer.ll					\
	glsl_parser.cc					\
	$(LIBGLSL_FILES)
builtincompiler_SOURCES = \
	$(top_srcdir)/src/mesa/main/hash_table.c	\
	$(top_srcdir)/src/mesa/program/prog_hash_table.c\
	$(top_srcdir)/src/mesa/program/symbol_table.c	\
	$(top_srcdir)/src/glsl/builtin_compiler/builtin_stubs.cpp \
	$(GLSL_COMPILER_CXX_FILES)
builtincompiler_LDADD = libglslcore.la glcpp/libglcpp.la
endif

# automake <=1.11 and automake >=1.12 have different conventions for naming C++ header files
# made by yacc.  To work with both, we write our own rule rather than using automake's.
# When (if) we require automake >=1.12 in configure.ac, this can be removed, and we can use
# the automake rule.
glsl_parser.cc glsl_parser.h: glsl_parser.yy
	$(AM_V_GEN) $(YACC) -v -o glsl_parser.cc -p "_mesa_glsl_" --defines=glsl_parser.h $<

BUILT_SOURCES = glsl_parser.h
CLEANFILES = glsl_lexer.cc glsl_parser.cc $(BUILT_SOURCES) builtin_function.cpp

builtin_function.cpp: $(srcdir)/builtins/profiles/* $(srcdir)/builtins/ir/* $(srcdir)/builtins/glsl/* $(srcdir)/builtins/tools/generate_builtins.py $(srcdir)/builtins/tools/texture_builtins.py $(BUILTIN_COMPILER)
	$(AM_V_GEN) $(PYTHON2) $(PYTHON_FLAGS) $(srcdir)/builtins/tools/generate_builtins.py $(builddir)/$(BUILTIN_COMPILER) > builtin_function.cpp || rm -f builtin_function.cpp
