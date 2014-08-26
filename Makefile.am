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

SUBDIRS = src

ACLOCAL_AMFLAGS = -I m4

doxygen:
	cd doxygen && $(MAKE)

.PHONY: doxygen

# Rules for making release tarballs

PACKAGE_DIR = Mesa-$(PACKAGE_VERSION)
PACKAGE_NAME = MesaLib-$(PACKAGE_VERSION)

EXTRA_FILES = \
	aclocal.m4					\
	configure					\
	bin/ar-lib					\
	bin/compile					\
	bin/config.sub					\
	bin/config.guess				\
	bin/depcomp					\
	bin/install-sh					\
	bin/ltmain.sh					\
	bin/missing					\
	bin/ylwrap					\
        bin/test-driver					\
	src/glsl/glsl_parser.cpp			\
	src/glsl/glsl_parser.h				\
	src/glsl/glsl_lexer.cpp				\
	src/glsl/glcpp/glcpp-lex.c			\
	src/glsl/glcpp/glcpp-parse.c			\
	src/glsl/glcpp/glcpp-parse.h			\
	src/mesa/program/lex.yy.c			\
	src/mesa/program/program_parse.tab.c		\
	src/mesa/program/program_parse.tab.h		\
	`git ls-files | grep "Makefile.am" | sed -e "s/Makefile.am/Makefile.in/"`


IGNORE_FILES = \
	-x autogen.sh


parsers: configure
	$(MAKE) -C src/glsl glsl_parser.cpp glsl_parser.h glsl_lexer.cpp glcpp/glcpp-lex.c glcpp/glcpp-parse.c glcpp/glcpp-parse.h

# Everything for new a Mesa release:
ARCHIVES = $(PACKAGE_NAME).tar.gz \
	$(PACKAGE_NAME).tar.bz2 \
	$(PACKAGE_NAME).zip

tarballs: checksums
	rm -f ../$(PACKAGE_DIR) $(PACKAGE_NAME).tar

manifest.txt: .git
	( \
		ls -1 $(EXTRA_FILES) ; \
		git ls-files $(IGNORE_FILES) \
	) | sed -e '/^\(.*\/\)\?\./d' -e "s@^@$(PACKAGE_DIR)/@" > $@

../$(PACKAGE_DIR):
	ln -s $(PWD) $@

$(PACKAGE_NAME).tar: parsers ../$(PACKAGE_DIR) manifest.txt
	cd .. ; tar -cf $(PACKAGE_DIR)/$(PACKAGE_NAME).tar -T $(PACKAGE_DIR)/manifest.txt

$(PACKAGE_NAME).tar.gz: $(PACKAGE_NAME).tar ../$(PACKAGE_DIR)
	gzip --stdout --best $(PACKAGE_NAME).tar > $(PACKAGE_NAME).tar.gz

$(PACKAGE_NAME).tar.bz2: $(PACKAGE_NAME).tar
	bzip2 --stdout --best $(PACKAGE_NAME).tar > $(PACKAGE_NAME).tar.bz2

$(PACKAGE_NAME).zip: parsers ../$(PACKAGE_DIR) manifest.txt
	rm -f $(PACKAGE_NAME).zip ; \
	cd .. ; \
	zip -q -@ $(PACKAGE_NAME).zip < $(PACKAGE_DIR)/manifest.txt ; \
	mv $(PACKAGE_NAME).zip $(PACKAGE_DIR)

checksums: $(ARCHIVES)
	@-sha256sum $(PACKAGE_NAME).tar.gz
	@-sha256sum $(PACKAGE_NAME).tar.bz2
	@-sha256sum $(PACKAGE_NAME).zip

.PHONY: tarballs checksums
