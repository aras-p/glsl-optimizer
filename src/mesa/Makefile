# src/mesa/Makefile

TOP = ../..
include $(TOP)/configs/current

MESA_LIBS := libmesa.a libmesagallium.a
DEPENDS := depend

SRCDIR = .
include sources.mak

# define preprocessor flags
MESA_CPPFLAGS := $(API_DEFINES) $(DEFINES)

# append include dirs
MESA_CPPFLAGS += $(INCLUDE_DIRS)

# tidy compiler flags
CFLAGS := $(filter-out $(DEFINES), $(CFLAGS))
CXXFLAGS := $(filter-out $(DEFINES), $(CXXFLAGS))

# LLVM is needed for the state tracker
MESA_CFLAGS := $(LLVM_CFLAGS) $(CFLAGS)
MESA_CXXFLAGS := $(LLVM_CFLAGS) $(CXXFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(MESA_CPPFLAGS) $(MESA_CFLAGS)

%.o: %.cpp
	$(CXX) -c -o $@ $< $(MESA_CPPFLAGS) $(MESA_CXXFLAGS)

%.o: %.S
	$(CC) -c -o $@ $< $(MESA_CPPFLAGS) $(MESA_CFLAGS)

# Default: build dependencies, then asm_subdirs, GLSL built-in lib,
# then convenience libs (.a) and finally the device drivers:
default: $(DEPENDS) asm_subdirs $(MESA_LIBS) driver_subdirs

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
GLAPI := $(TOP)/src/mapi/glapi/gen
include $(GLAPI)/glapi_gen.mk

BUILT_SOURCES = \
	main/api_exec_es1_dispatch.h \
	main/api_exec_es1_remap_helper.h \
	main/api_exec_es2_dispatch.h \
	main/api_exec_es2_remap_helper.h \
	main/api_exec_es1.c \
	main/api_exec_es2.c \
	program/program_parse.tab.c \
	program/program_parse.tab.h \
	program/lex.yy.c

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
	$(BISON) -v -d --output=program/program_parse.tab.c $<

program/lex.yy.c: program/program_lexer.l
	$(FLEX) --never-interactive --outfile=$@ $<

ifneq (,$(DRICORE_LIBS))
DRICORE_TARGET = dricore
DRICORE_INSTALL_TARGET = install-dricore
endif

######################################################################
# Helper libraries used by many drivers:

# Make archive of core mesa object files
libmesa.a: $(MESA_OBJECTS) $(GLSL_LIBS)
	@ $(MKLIB) -o mesa -static $(MESA_OBJECTS) $(GLSL_LIBS)

# Make archive of subset of core mesa object files for gallium
libmesagallium.a: $(MESA_GALLIUM_OBJECTS) $(GLSL_LIBS)
	@ $(MKLIB) -o mesagallium -static $(MESA_GALLIUM_OBJECTS) $(GLSL_LIBS)

dricore: $(BUILT_SOURCES)
	@ (cd libdricore && $(MAKE))

######################################################################
# Device drivers
driver_subdirs: $(MESA_LIBS) $(DRICORE_TARGET)
	@ (cd drivers && $(MAKE))


######################################################################
# Assembly subdirs
asm_subdirs:
	@ if echo "$(DEFINES)" | grep -q USE_X86_ASM ; then \
		(cd x86 && $(MAKE)) || exit 1 ; \
	fi
	@ if echo "$(DEFINES)" | grep -q USE_X86_64_ASM ; then \
		(cd x86-64 && $(MAKE)) || exit 1 ; \
	fi

######################################################################
# Dependency generation

depend: $(ALL_FILES) main/git_sha1.h
	@ echo "running $(MKDEP)"
	@ touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) $(MESA_CPPFLAGS) \
		$(ALL_FILES) > /dev/null 2>/dev/null

######################################################################
# Installation rules

# this isn't fleshed out yet but is probably the way to go in the future
new_install:
	(cd drivers && $(MAKE) install)

# XXX replace this with new_install above someday
install: default $(DRICORE_INSTALL_TARGET)
	@for driver in $(DRIVER_DIRS) ; do \
	  case "$$driver" in \
	    osmesa) if [ "$(DRIVER_DIRS)" = osmesa ]; then \
	              $(MAKE) install-headers install-osmesa || exit 1 ; \
	            else \
	              $(MAKE) install-osmesa || exit 1 ; \
	            fi ;; \
	    dri)    $(MAKE) install-libgl-pc install-dri || exit 1 ;; \
	    *)      $(MAKE) install-libgl-pc || exit 1 ;; \
	  esac ; \
	done

pcedit = \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),' \
	-e 's,@INSTALL_LIB_DIR@,$(INSTALL_LIB_DIR),' \
	-e 's,@INSTALL_INC_DIR@,$(INSTALL_INC_DIR),' \
	-e 's,@VERSION@,$(MESA_MAJOR).$(MESA_MINOR).$(MESA_TINY),' \

osmesa_pcedit = sed \
	$(pcedit) \
	-e 's,@OSMESA_LIB@,$(OSMESA_LIB),' \
	-e 's,@OSMESA_PC_REQ@,$(OSMESA_PC_REQ),' \
	-e 's,@OSMESA_PC_LIB_PRIV@,$(OSMESA_PC_LIB_PRIV),'

osmesa.pc: osmesa.pc.in
	$(osmesa_pcedit) $< > $@

install-headers:
	$(INSTALL) -d $(DESTDIR)$(INSTALL_INC_DIR)/GL
	$(INSTALL) -m 644 $(TOP)/include/GL/*.h \
		$(DESTDIR)$(INSTALL_INC_DIR)/GL

install-libgl-pc: install-headers
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig
	$(INSTALL) -m 644 gl.pc $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig

install-osmesa: default osmesa.pc
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig
	$(MINSTALL) $(TOP)/$(LIB_DIR)/$(OSMESA_LIB_GLOB) \
		$(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -m 644 osmesa.pc $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig

install-dri: default
	cd drivers/dri && $(MAKE) install

install-dricore: default
	@ (cd libdricore && $(MAKE) install)

# Emacs tags
tags:
	etags `find . -name \*.[ch]` $(TOP)/include/GL/*.h

clean:
	-rm -f */*.o
	-rm -f */*/*.o
	-rm -f depend depend.bak libmesa.a libmesagallium.a
	-rm -f drivers/*/*.o
	-rm -f osmesa.pc
	-rm -f $(BUILT_SOURCES)
	-@cd drivers/dri && $(MAKE) clean
	-@cd drivers/x11 && $(MAKE) clean
	-@cd drivers/osmesa && $(MAKE) clean
	-@cd x86 && $(MAKE) clean
	-@cd x86-64 && $(MAKE) clean
	-@cd libdricore && $(MAKE) clean


-include $(DEPENDS)
