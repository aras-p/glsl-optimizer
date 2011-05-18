# src/mesa/Makefile

TOP = ../..
include $(TOP)/configs/current

MESA_LIBS := libmesa.a libmesagallium.a
DEPENDS := depend

MESA_OBJ_DIR := .
DRICORE_OBJ_DIR := objs-dricore

include sources.mak

# adjust object dirs
MESA_OBJECTS := $(addprefix $(MESA_OBJ_DIR)/, $(MESA_OBJECTS))
MESA_GALLIUM_OBJECTS := $(addprefix $(MESA_OBJ_DIR)/, $(MESA_GALLIUM_OBJECTS))

DRICORE_OBJECTS := $(addprefix $(DRICORE_OBJ_DIR)/, $(MESA_OBJECTS))

# define preprocessor flags
MESA_CPPFLAGS := $(API_DEFINES) $(DEFINES)

# append include dirs
MESA_CPPFLAGS += $(INCLUDE_DIRS)

DRICORE_CPPFLAGS = $(MESA_CPPFLAGS)

# tidy compiler flags
CFLAGS := $(filter-out $(DEFINES), $(CFLAGS))
CXXFLAGS := $(filter-out $(DEFINES), $(CXXFLAGS))

# LLVM is needed for the state tracker
MESA_CFLAGS := $(LLVM_CFLAGS) $(CFLAGS)
DRICORE_CFLAGS := $(LLVM_CFLAGS) $(DRI_CFLAGS)

MESA_CXXFLAGS := $(LLVM_CFLAGS) $(CXXFLAGS)
DRICORE_CXXFLAGS := $(LLVM_CFLAGS) $(DRI_CXXFLAGS)

define mesa-cc-c
	@mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $($(1)_CPPFLAGS) $($(1)_CFLAGS)
endef

define mesa-cxx-c
	@mkdir -p $(dir $@)
	$(CXX) -c -o $@ $< $($(1)_CPPFLAGS) $($(1)_CXXFLAGS)
endef

$(MESA_OBJ_DIR)/%.o: %.c
	$(call mesa-cc-c,MESA)

$(MESA_OBJ_DIR)/%.o: %.cpp
	$(call mesa-cxx-c,MESA)

$(MESA_OBJ_DIR)/%.o: %.S
	$(call mesa-cc-c,MESA)

$(DRICORE_OBJ_DIR)/%.o: %.c
	$(call mesa-cc-c,DRICORE)

$(DRICORE_OBJ_DIR)/%.o: %.cpp
	$(call mesa-cxx-c,DRICORE)

$(DRICORE_OBJ_DIR)/%.o: %.S
	$(call mesa-cc-c,DRICORE)

# Default: build dependencies, then asm_subdirs, GLSL built-in lib,
# then convenience libs (.a) and finally the device drivers:
default: $(DEPENDS) asm_subdirs $(MESA_LIBS) $(DRICORE_LIBS) driver_subdirs

main/api_exec_es1.c: main/APIspec.xml main/es_generator.py main/APIspecutil.py main/APIspec.py
	$(PYTHON2) $(PYTHON_FLAGS) main/es_generator.py -S main/APIspec.xml -V GLES1.1 > $@

main/api_exec_es2.c: main/APIspec.xml main/es_generator.py main/APIspecutil.py main/APIspec.py
	$(PYTHON2) $(PYTHON_FLAGS) main/es_generator.py -S main/APIspec.xml -V GLES2.0 > $@

program/program_parse.tab.c program/program_parse.tab.h: program/program_parse.y
	$(BISON) -v -d --output=program/program_parse.tab.c $<

program/lex.yy.c: program/program_lexer.l
	$(FLEX) --never-interactive --outfile=$@ $<

######################################################################
# Helper libraries used by many drivers:

# Make archive of core mesa object files
libmesa.a: $(MESA_OBJECTS) $(GLSL_LIBS)
	@ $(MKLIB) -o mesa -static $(MESA_OBJECTS) $(GLSL_LIBS)

# Shared dricore library for classic DRI drivers
$(TOP)/$(LIB_DIR)/libdricore.so: $(DRICORE_OBJECTS) $(DRICORE_GLSL_LIBS)
	@$(MKLIB) -o $@ -linker '$(CXX)' -ldflags '$(LDFLAGS)' \
		-cplusplus -noprefix \
		-install $(TOP)/$(LIB_DIR) -id $(DRI_DRIVER_INSTALL_DIR)/$@.dylib \
		$(DRICORE_LIB_DEPS) $(DRICORE_OBJECTS)

# Make archive of subset of core mesa object files for gallium
libmesagallium.a: $(MESA_GALLIUM_OBJECTS) $(GLSL_LIBS)
	@ $(MKLIB) -o mesagallium -static $(MESA_GALLIUM_OBJECTS) $(GLSL_LIBS)

######################################################################
# Device drivers
driver_subdirs: $(MESA_LIBS) $(DRICORE_LIBS)
	@ (cd drivers && $(MAKE))


######################################################################
# Assembly subdirs
asm_subdirs:
	@ if echo "$(ASM_FLAGS)" | grep -q USE_X86_ASM ; then \
		(cd x86 && $(MAKE)) || exit 1 ; \
	fi
	@ if echo "$(ASM_FLAGS)" | grep -q USE_X86_64_ASM ; then \
		(cd x86 && $(MAKE)) || exit 1 ; \
		(cd x86-64 && $(MAKE)) || exit 1 ; \
	fi


######################################################################
# Dependency generation

depend: $(ALL_SOURCES)
	@ echo "running $(MKDEP)"
	@ touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) -p$(MESA_OBJ_DIR)/ $(MESA_CPPFLAGS) \
		$(ALL_SOURCES) > /dev/null 2>/dev/null

######################################################################
# Installation rules

# this isn't fleshed out yet but is probably the way to go in the future
new_install:
	(cd drivers && $(MAKE) install)

ifneq (,$(DRICORE_LIBS))
DRICORE_INSTALL_TARGET = install-dricore
endif

# XXX replace this with new_install above someday
install: default $(DRICORE_INSTALL_TARGET)
	@for driver in $(DRIVER_DIRS) ; do \
	  case "$$driver" in \
	    osmesa) if [ "$(DRIVER_DIRS)" = osmesa ]; then \
	              $(MAKE) install-headers install-osmesa || exit 1 ; \
	            else \
	              $(MAKE) install-osmesa || exit 1 ; \
	            fi ;; \
	    dri)    $(MAKE) install-libgl install-dri || exit 1 ;; \
	    *)      $(MAKE) install-libgl || exit 1 ;; \
	  esac ; \
	done

pcedit = \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),' \
	-e 's,@INSTALL_LIB_DIR@,$(INSTALL_LIB_DIR),' \
	-e 's,@INSTALL_INC_DIR@,$(INSTALL_INC_DIR),' \
	-e 's,@VERSION@,$(MESA_MAJOR).$(MESA_MINOR).$(MESA_TINY),' \


gl_pcedit = sed \
	$(pcedit) \
	-e 's,@GL_PC_REQ_PRIV@,$(GL_PC_REQ_PRIV),' \
	-e 's,@GL_PC_LIB_PRIV@,$(GL_PC_LIB_PRIV),' \
	-e 's,@GL_PC_CFLAGS@,$(GL_PC_CFLAGS),' \
	-e 's,@GLX_TLS@,$(GLX_TLS),' \
	-e 's,@GL_LIB@,$(GL_LIB),'

gl.pc: gl.pc.in
	$(gl_pcedit) $< > $@

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

install-libgl: default gl.pc install-headers
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig
	$(MINSTALL) $(TOP)/$(LIB_DIR)/$(GL_LIB_GLOB) \
		$(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -m 644 gl.pc $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig

install-osmesa: default osmesa.pc
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig
	$(MINSTALL) $(TOP)/$(LIB_DIR)/$(OSMESA_LIB_GLOB) \
		$(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -m 644 osmesa.pc $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig

install-dri: default
	cd drivers/dri && $(MAKE) install

# We don't need MINSTALL here because we're not installing symbolic links
install-dricore: default
	$(INSTALL) -d $(DESTDIR)$(DRI_DRIVER_INSTALL_DIR)
	$(INSTALL) -m 755 $(DRICORE_LIBS) $(DESTDIR)$(DRI_DRIVER_INSTALL_DIR)


# Emacs tags
tags:
	etags `find . -name \*.[ch]` $(TOP)/include/GL/*.h

clean-dricore:
	-rm -f libdricore.so
	-rm -f $(DRICORE_LIBS)
	-rm -rf $(DRICORE_OBJ_DIR)

clean: clean-dricore
	-rm -f */*.o
	-rm -f */*/*.o
	-rm -f depend depend.bak libmesa.a libmesagallium.a
	-rm -f drivers/*/*.o
	-rm -f *.pc
	-@cd drivers/dri && $(MAKE) clean
	-@cd drivers/x11 && $(MAKE) clean
	-@cd drivers/osmesa && $(MAKE) clean
	-@cd x86 && $(MAKE) clean
	-@cd x86-64 && $(MAKE) clean


-include $(DEPENDS)
