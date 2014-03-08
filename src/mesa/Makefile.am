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

SUBDIRS = . main/tests

if HAVE_X11_DRIVER
SUBDIRS += drivers/x11
endif

if HAVE_DRI
SUBDIRS += drivers/dri
endif

if HAVE_OSMESA
SUBDIRS += drivers/osmesa
endif

gldir = $(includedir)/GL
gl_HEADERS = $(top_srcdir)/include/GL/*.h

.PHONY: $(BUILDDIR)main/git_sha1.h.tmp
$(BUILDDIR)main/git_sha1.h.tmp:
	@touch main/git_sha1.h.tmp
	@if test -d $(top_srcdir)/.git; then \
		if which git > /dev/null; then \
		    git --git-dir=$(top_srcdir)/.git log -n 1 --oneline | \
			sed 's/^\([^ ]*\) .*/#define MESA_GIT_SHA1 "git-\1"/' \
			> main/git_sha1.h.tmp ; \
		fi \
	fi

$(BUILDDIR)main/git_sha1.h: $(BUILDDIR)main/git_sha1.h.tmp
	@echo "updating main/git_sha1.h"
	@if ! cmp -s main/git_sha1.h.tmp main/git_sha1.h; then \
		mv main/git_sha1.h.tmp main/git_sha1.h ;\
	else \
		rm main/git_sha1.h.tmp ;\
	fi

# include glapi_gen.mk for generating glapi headers for GLES
GLAPI = $(top_srcdir)/src/mapi/glapi/gen
include $(GLAPI)/glapi_gen.mk

BUILDDIR = $(builddir)/
include Makefile.sources

BUILT_SOURCES = \
	main/get_hash.h \
	$(BUILDDIR)main/git_sha1.h \
	$(BUILDDIR)program/program_parse.tab.c \
	$(BUILDDIR)program/lex.yy.c
CLEANFILES = \
	$(BUILT_SOURCES) \
	$(BUILDDIR)program/program_parse.tab.h \
	$(BUILDDIR)main/git_sha1.h.tmp

GET_HASH_GEN = main/get_hash_generator.py

main/get_hash.h: $(GLAPI)/gl_and_es_API.xml main/get_hash_params.py 	\
		 $(GET_HASH_GEN) Makefile
	$(AM_V_GEN)set -e;						\
	$(PYTHON2) $(PYTHON_FLAGS) $(srcdir)/$(GET_HASH_GEN)		\
		-f $< > $@.tmp;						\
	mv $@.tmp $@;

noinst_LTLIBRARIES = $(ARCH_LIBS)
if NEED_LIBMESA
noinst_LTLIBRARIES += libmesa.la
else
check_LTLIBRARIES = libmesa.la
endif
if HAVE_GALLIUM
noinst_LTLIBRARIES += libmesagallium.la
endif

AM_CPPFLAGS = $(DEFINES) $(INCLUDE_DIRS)
AM_CFLAGS = $(LLVM_CFLAGS) $(VISIBILITY_CFLAGS)
AM_CXXFLAGS = $(LLVM_CFLAGS) $(VISIBILITY_CXXFLAGS)

ARCH_LIBS =

if SSE41_SUPPORTED
ARCH_LIBS += libmesa_sse41.la
endif

MESA_ASM_FILES_FOR_ARCH =

if HAVE_X86_ASM
noinst_PROGRAMS = gen_matypes

gen_matypes_SOURCES = x86/gen_matypes.c
BUILT_SOURCES += matypes.h

if HAVE_X86_64_ASM
MESA_ASM_FILES_FOR_ARCH += $(X86_64_FILES)
AM_CPPFLAGS += -I$(builddir)/x86-64 -I$(srcdir)/x86-64
else
MESA_ASM_FILES_FOR_ARCH += $(X86_FILES)
AM_CPPFLAGS += -I$(builddir)/x86 -I$(srcdir)/x86
endif
endif
if HAVE_SPARC_ASM
MESA_ASM_FILES_FOR_ARCH += $(SPARC_FILES)
AM_CPPFLAGS += -I$(builddir)/sparc -I$(srcdir)/sparc
endif

libmesa_la_SOURCES = \
	$(MESA_FILES) \
	$(PROGRAM_FILES) \
        $(MESA_ASM_FILES_FOR_ARCH)

libmesa_la_LIBADD = \
        $(top_builddir)/src/glsl/libglsl.la \
	$(ARCH_LIBS) \
        $()

libmesagallium_la_SOURCES = \
	$(MESA_GALLIUM_FILES) \
	$(PROGRAM_FILES) \
        $(MESA_ASM_FILES_FOR_ARCH)

libmesagallium_la_LIBADD = \
        $(top_builddir)/src/glsl/libglsl.la \
	$(ARCH_LIBS) \
        $()

libmesa_sse41_la_SOURCES = \
	main/streaming-load-memcpy.c
libmesa_sse41_la_CFLAGS = $(AM_CFLAGS) -msse4.1

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gl.pc

$(BUILDDIR)program/lex.yy.c: program/program_lexer.l
	$(AM_V_at)$(MKDIR_P) program
	$(AM_V_GEN) $(LEX) --never-interactive --outfile=$@ $<

$(BUILDDIR)program/program_parse.tab.c $(BUILDDIR)program/program_parse.tab.h: program/program_parse.y
	$(AM_V_at)$(MKDIR_P) program
	$(AM_V_GEN) $(YACC) -p "_mesa_program_" -v -d --output=$(BUILDDIR)program/program_parse.tab.c $<

if GEN_ASM_OFFSETS
matypes.h: $(gen_matypes_SOURCES)
	$(AM_V_GEN)$(COMPILE) $< -DASM_OFFSETS -S -o - | \
		sed -n '/^->/{s:^->::;/[$$]/{s:^:#define :;s:[$$]::};p}' > $@
else
matypes.h: gen_matypes
	$(AM_V_GEN)./gen_matypes > $@
endif

# Emacs tags
tags:
	etags `find . -name \*.[ch]` $(top_srcdir)/include/GL/*.h
