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

SUBDIRS = x86 x86-64 . libdricore drivers

gldir = $(includedir)/GL
gl_HEADERS = $(top_srcdir)/include/GL/*.h

.PHONY: main/git_sha1.h.tmp
main/git_sha1.h.tmp:
	@touch main/git_sha1.h.tmp
	@if test -d ../../.git; then \
		if which git > /dev/null; then \
		    git log -n 1 --oneline | \
			sed 's/^\([^ ]*\) .*/#define MESA_GIT_SHA1 "git-\1"/' \
			> main/git_sha1.h.tmp ; \
		fi \
	fi

main/git_sha1.h: main/git_sha1.h.tmp
	@echo "updating main/git_sha1.h"
	@if ! cmp -s main/git_sha1.h.tmp main/git_sha1.h; then \
		mv main/git_sha1.h.tmp main/git_sha1.h ;\
	fi

# include glapi_gen.mk for generating glapi headers for GLES
TOP = $(top_srcdir)
GLAPI = $(TOP)/src/mapi/glapi/gen
include $(GLAPI)/glapi_gen.mk

BUILT_SOURCES = \
	main/git_sha1.h \
	main/api_exec_es1_dispatch.h \
	main/api_exec_es1_remap_helper.h \
	main/api_exec_es2_dispatch.h \
	main/api_exec_es2_remap_helper.h \
	main/api_exec_es1.c \
	main/api_exec_es2.c \
	program/program_parse.tab.c \
	program/program_parse.tab.h \
	program/lex.yy.c
CLEANFILES = \
	$(BUILT_SOURCES) \
	git_sha1.h.tmp

main/api_exec_es1_dispatch.h: $(GLAPI)/gl_and_es_API.xml $(glapi_gen_dispatch_deps)
	$(call glapi_gen_dispatch,$<,es1)

main/api_exec_es1_remap_helper.h: $(GLAPI)/gl_and_es_API.xml $(glapi_gen_remap_deps)
	$(call glapi_gen_remap,$<,es1)

main/api_exec_es1.o: main/api_exec_es1_dispatch.h main/api_exec_es1_remap_helper.h

main/api_exec_es2_dispatch.h: $(GLAPI)/gl_and_es_API.xml $(glapi_gen_dispatch_deps)
	$(call glapi_gen_dispatch,$<,es2)

main/api_exec_es2_remap_helper.h: $(GLAPI)/gl_and_es_API.xml $(glapi_gen_remap_deps)
	$(call glapi_gen_remap,$<,es2)

main/api_exec_es2.o: main/api_exec_es2_dispatch.h main/api_exec_es2_remap_helper.h

main/api_exec_es1.c: main/APIspec.xml main/es_generator.py main/APIspecutil.py main/APIspec.py
	$(PYTHON2) $(PYTHON_FLAGS) main/es_generator.py -S main/APIspec.xml -V GLES1.1 > $@

main/api_exec_es2.c: main/APIspec.xml main/es_generator.py main/APIspecutil.py main/APIspec.py
	$(PYTHON2) $(PYTHON_FLAGS) main/es_generator.py -S main/APIspec.xml -V GLES2.0 > $@

program/program_parse.tab.c program/program_parse.tab.h: program/program_parse.y
	$(YACC) -v -d --output=program/program_parse.tab.c $<

program/lex.yy.c: program/program_lexer.l
	$(LEX) --never-interactive --outfile=$@ $<

all-local:
	$(MAKE) -f Makefile.old

install-exec-local:
	$(MAKE) -f Makefile.old install

clean-local:
	$(MAKE) -f Makefile.old clean

pkgconfigdir = $(libdir)/pkgconfig

if HAVE_OSMESA_DRIVER
pkgconfig_DATA = osmesa.pc
else
pkgconfig_DATA = gl.pc
endif
