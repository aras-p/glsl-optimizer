# Top-level Mesa makefile

TOP = .

SUBDIRS = src


# The git command below generates an empty string when we're not
# building in a GIT tree (i.e., building from a release tarball).
default: $(TOP)/configs/current
	@$(TOP)/bin/extract_git_sha1
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE)) || exit 1 ; \
		fi \
	done

all: default


doxygen:
	cd doxygen && $(MAKE)

check:
	make -C src/glsl/tests check
	make -C tests check

clean:
	-@touch $(TOP)/configs/current
	-@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) clean) ; \
		fi \
	done
	-@test -s $(TOP)/configs/current || rm -f $(TOP)/configs/current


realclean: clean
	-rm -rf lib*
	-rm -f $(TOP)/configs/current
	-rm -f $(TOP)/configs/autoconf
	-rm -rf autom4te.cache
	-find . '(' -name '*.o' -o -name '*.a' -o -name '*.so' -o \
	  -name depend -o -name depend.bak ')' -exec rm -f '{}' ';'


distclean: realclean


install:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) install) || exit 1 ; \
		fi \
	done


.PHONY: default doxygen clean realclean distclean install check

# If there's no current configuration file
$(TOP)/configs/current:
	@echo
	@echo
	@echo "Please run './configure' then 'make'"
	@echo "See './configure --help' for details"
	@echo
	@echo "(ignore the following error message)"
	@exit 1

# Rules for making release tarballs

PACKAGE_VERSION=8.1-devel
PACKAGE_DIR = Mesa-$(PACKAGE_VERSION)
PACKAGE_NAME = MesaLib-$(PACKAGE_VERSION)

EXTRA_FILES = \
	aclocal.m4					\
	configure					\
	tests/Makefile.in				\
	tests/glx/Makefile.in				\
	src/glsl/glsl_parser.cpp			\
	src/glsl/glsl_parser.h				\
	src/glsl/glsl_lexer.cpp				\
	src/glsl/glcpp/glcpp-lex.c			\
	src/glsl/glcpp/glcpp-parse.c			\
	src/glsl/glcpp/glcpp-parse.h			\
	src/mesa/main/api_exec_es1.c			\
	src/mesa/main/api_exec_es1_dispatch.h		\
	src/mesa/main/api_exec_es1_remap_helper.h	\
	src/mesa/main/api_exec_es2.c			\
	src/mesa/main/api_exec_es2_dispatch.h		\
	src/mesa/main/api_exec_es2_remap_helper.h	\
	src/mesa/program/lex.yy.c			\
	src/mesa/program/program_parse.tab.c		\
	src/mesa/program/program_parse.tab.h

IGNORE_FILES = \
	-x autogen.sh


parsers: configure
	-@touch $(TOP)/configs/current
	$(MAKE) -C src/glsl glsl_parser.cpp glsl_parser.h glsl_lexer.cpp
	$(MAKE) -C src/glsl/glcpp glcpp-lex.c glcpp-parse.c glcpp-parse.h
	$(MAKE) -C src/mesa program/lex.yy.c program/program_parse.tab.c program/program_parse.tab.h

# Everything for new a Mesa release:
ARCHIVES = $(PACKAGE_NAME).tar.gz \
	$(PACKAGE_NAME).tar.bz2 \
	$(PACKAGE_NAME).zip \

tarballs: md5
	rm -f ../$(PACKAGE_DIR) $(PACKAGE_NAME).tar

# Helper for autoconf builds
ACLOCAL = aclocal
ACLOCAL_FLAGS =
AUTOCONF = autoconf
AC_FLAGS =
aclocal.m4: configure.ac acinclude.m4
	$(ACLOCAL) $(ACLOCAL_FLAGS)
configure: configure.ac aclocal.m4 acinclude.m4
	$(AUTOCONF) $(AC_FLAGS)

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

md5: $(ARCHIVES)
	@-md5sum $(PACKAGE_NAME).tar.gz
	@-md5sum $(PACKAGE_NAME).tar.bz2
	@-md5sum $(PACKAGE_NAME).zip

am--refresh:

.PHONY: tarballs md5 am--refresh
