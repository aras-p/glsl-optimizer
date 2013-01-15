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
SUBDIRS = builtin_compiler
endif

AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/src/mesa/ \
	-I$(top_srcdir)/src/glsl/glcpp \
	$(DEFINES) \
	$(API_DEFINES)
AM_CFLAGS = $(VISIBILITY_CFLAGS)
AM_CXXFLAGS = $(VISIBILITY_CXXFLAGS)

include Makefile.sources

TESTS = glcpp/tests/glcpp-test

noinst_LTLIBRARIES = libglcpp.la libglsl.la
check_PROGRAMS = glcpp/glcpp glsl_test

libglcpp_la_SOURCES =					\
	glcpp/glcpp-lex.c				\
	glcpp/glcpp-parse.c				\
	$(LIBGLCPP_FILES)

glcpp_glcpp_SOURCES =					\
	glcpp/glcpp.c					\
	$(top_srcdir)/src/mesa/program/prog_hash_table.c
glcpp_glcpp_LDADD = libglcpp.la

libglsl_la_SOURCES =					\
	glsl_lexer.cpp					\
	glsl_parser.cpp					\
	$(LIBGLSL_FILES)				\
	builtin_function.cpp
libglsl_la_LIBADD = libglcpp.la

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
	glsl_lexer.cpp					\
	glsl_parser.cpp					\
	$(LIBGLSL_FILES)
builtincompiler_SOURCES = \
	$(top_srcdir)/src/mesa/main/hash_table.c	\
	$(top_srcdir)/src/mesa/program/prog_hash_table.c\
	$(top_srcdir)/src/mesa/program/symbol_table.c	\
	$(top_srcdir)/src/glsl/builtin_compiler/builtin_stubs.cpp \
	$(GLSL_COMPILER_CXX_FILES)
builtincompiler_LDADD = libglslcore.la libglcpp.la
endif

# We write our own rules for yacc and lex below. We'd rather use automake,
# but automake makes it especially difficult for a number of reasons:
#
#  * < automake-1.12 generates .h files from .yy and .ypp files, but
#    >=automake-1.12 generates .hh and .hpp files respectively. There's no
#    good way of making a project that uses C++ yacc files compatible with
#    both versions of automake. Strong work automake developers.
#
#  * Since we're generating code from .l/.y files in a subdirectory (glcpp/)
#    we'd like the resulting generated code to also go in glcpp/ for purposes
#    of distribution. Automake gives no way to do this.
#
#  * Since we're building multiple yacc parsers into one library (and via one
#    Makefile) we have to use per-target YFLAGS. Using per-target YFLAGS causes
#    automake to name the resulting generated code as <library-name>_filename.c.
#    Frankly, that's ugly and we don't want a libglcpp_glcpp_parser.h file.

# In order to make build output print "LEX" and "YACC", we reproduce the
# automake variables below.

AM_V_LEX = $(am__v_LEX_$(V))
am__v_LEX_ = $(am__v_LEX_$(AM_DEFAULT_VERBOSITY))
am__v_LEX_0 = @echo "  LEX     " $@;
am__v_LEX_1 =

AM_V_YACC = $(am__v_YACC_$(V))
am__v_YACC_ = $(am__v_YACC_$(AM_DEFAULT_VERBOSITY))
am__v_YACC_0 = @echo "  YACC    " $@;
am__v_YACC_1 =

glsl_parser.cpp glsl_parser.h: glsl_parser.yy
	$(AM_V_YACC) $(YACC) $(YFLAGS) -o $@ -p "_mesa_glsl_" --defines=$(GLSL_BUILDDIR)/glsl_parser.h $<

glsl_lexer.cpp: glsl_lexer.ll
	$(AM_V_LEX) $(LEX) $(LFLAGS) -o $@ $<

glcpp/glcpp-parse.c glcpp/glcpp-parse.h: glcpp/glcpp-parse.y
	$(MKDIR_P) $(GLSL_BUILDDIR)/glcpp
	$(AM_V_YACC) $(YACC) $(YFLAGS) -o $@ -p "glcpp_parser_" --defines=$(GLSL_BUILDDIR)/glcpp/glcpp-parse.h $<

glcpp/glcpp-lex.c: glcpp/glcpp-lex.l
	$(MKDIR_P) $(GLSL_BUILDDIR)/glcpp
	$(AM_V_LEX) $(LEX) $(LFLAGS) -o $@ $<

# Only the parsers (specifically the header files generated at the same time)
# need to be in BUILT_SOURCES. Though if we list the parser headers YACC is
# called for the .c/.cpp file and the .h files. By listing the .c/.cpp files
# YACC is only executed once for each parser. The rest of the generated code
# will be created at the appropriate times according to standard automake
# dependency rules.
BUILT_SOURCES =						\
	glsl_parser.cpp					\
	glcpp/glcpp-parse.c
CLEANFILES =						\
	glcpp/glcpp-parse.h				\
	glcpp/glcpp-lex.c				\
	glsl_parser.h					\
	glsl_lexer.cpp					\
	$(BUILT_SOURCES)

builtin_function.cpp: $(srcdir)/builtins/profiles/* $(srcdir)/builtins/ir/* $(srcdir)/builtins/glsl/* $(srcdir)/builtins/tools/generate_builtins.py $(srcdir)/builtins/tools/texture_builtins.py $(BUILTIN_COMPILER)
	$(AM_V_GEN) $(PYTHON2) $(PYTHON_FLAGS) $(srcdir)/builtins/tools/generate_builtins.py $(builddir)/$(BUILTIN_COMPILER) > builtin_function.cpp || rm -f builtin_function.cpp
