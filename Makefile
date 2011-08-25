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
	cd src/glsl/tests/ && ./optimization-test
	@echo "All tests passed."

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
	@echo "Please choose a configuration from the following list:"
	@ls -1 $(TOP)/configs | grep -v "current\|default\|CVS\|autoconf.*"
	@echo
	@echo "Then type 'make <config>' (ex: 'make linux-x86')"
	@echo
	@echo "Or, run './configure' then 'make'"
	@echo "See './configure --help' for details"
	@echo
	@echo "(ignore the following error message)"
	@exit 1


# Rules to set/install a specific build configuration
aix \
aix-64 \
aix-64-static \
aix-gcc \
aix-static \
autoconf \
bluegene-osmesa \
bluegene-xlc-osmesa \
catamount-osmesa-pgi \
darwin \
darwin-fat-32bit \
darwin-fat-all \
freebsd \
freebsd-dri \
freebsd-dri-amd64 \
freebsd-dri-x86 \
hpux10 \
hpux10-gcc \
hpux10-static \
hpux11-32 \
hpux11-32-static \
hpux11-32-static-nothreads \
hpux11-64 \
hpux11-64-static \
hpux11-ia64 \
hpux11-ia64-static \
hpux9 \
hpux9-gcc \
irix6-64 \
irix6-64-static \
irix6-n32 \
irix6-n32-static \
irix6-o32 \
irix6-o32-static \
linux \
linux-i965 \
linux-alpha \
linux-alpha-static \
linux-cell \
linux-cell-debug \
linux-debug \
linux-dri \
linux-dri-debug \
linux-dri-x86 \
linux-dri-x86-64 \
linux-dri-ppc \
linux-dri-xcb \
linux-egl \
linux-indirect \
linux-fbdev \
linux-ia64-icc \
linux-ia64-icc-static \
linux-icc \
linux-icc-static \
linux-llvm \
linux-llvm-debug \
linux-opengl-es \
linux-osmesa \
linux-osmesa-static \
linux-osmesa16 \
linux-osmesa16-static \
linux-osmesa32 \
linux-ppc \
linux-ppc-static \
linux-profile \
linux-sparc \
linux-sparc5 \
linux-static \
linux-ultrasparc \
linux-tcc \
linux-x86 \
linux-x86-debug \
linux-x86-32 \
linux-x86-64 \
linux-x86-64-debug \
linux-x86-64-profile \
linux-x86-64-static \
linux-x86-profile \
linux-x86-static \
netbsd \
openbsd \
osf1 \
osf1-static \
solaris-x86 \
solaris-x86-gcc \
solaris-x86-gcc-static \
sunos4 \
sunos4-gcc \
sunos4-static \
sunos5 \
sunos5-gcc \
sunos5-64-gcc \
sunos5-smp \
sunos5-v8 \
sunos5-v8-static \
sunos5-v9 \
sunos5-v9-static \
sunos5-v9-cc-g++ \
ultrix-gcc:
	@ if test -f configs/current -o -L configs/current; then \
		if ! cmp configs/$@ configs/current > /dev/null; then \
			echo "Please run 'make realclean' before changing configs" ; \
			exit 1 ; \
		fi ; \
	else \
		cd configs && rm -f current && ln -s $@ current ; \
	fi
	$(MAKE) default


# Rules for making release tarballs

PACKAGE_VERSION=7.12-devel
PACKAGE_DIR = Mesa-$(PACKAGE_VERSION)
PACKAGE_NAME = MesaLib-$(PACKAGE_VERSION)

EXTRA_FILES = \
	aclocal.m4					\
	configure					\
	src/glsl/glsl_parser.cpp			\
	src/glsl/glsl_parser.h				\
	src/glsl/glsl_lexer.cpp				\
	src/glsl/glcpp/glcpp-lex.c			\
	src/glsl/glcpp/glcpp-parse.c			\
	src/glsl/glcpp/glcpp-parse.h			\
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

.PHONY: tarballs md5
