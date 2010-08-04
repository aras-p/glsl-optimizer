# Copyright © 2010 Intel Corporation
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# on the rights to use, copy, modify, merge, publish, distribute, sub
# license, and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
# AUTHORS, COPYRIGHT HOLDERS, AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
# USE OR OTHER DEALINGS IN THE SOFTWARE.

noinst_LTLIBRARIES = libglcpp.la
libglcpp_la_SOURCES = \
	glcpp-lex.l \
	glcpp-parse.y \
	glcpp.h \
	pp.c

BUILT_SOURCES = glcpp-parse.h glcpp-parse.c glcpp-lex.c
CLEANFILES = $(BUILT_SOURCES)

glcpp-parse.h: glcpp-parse.c

bin_PROGRAMS = glcpp
glcpp_LDADD = libglcpp.la
glcpp_LDFLAGS = @LDFLAGS@ $(talloc_LIBS)
glcpp_SOURCES = glcpp.c

.l.c:
	$(LEXCOMPILE) --outfile="$@" $<

test: glcpp
	@(cd tests; ./glcpp-test)
