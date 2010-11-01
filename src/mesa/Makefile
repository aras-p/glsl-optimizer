# src/mesa/Makefile

TOP = ../..
include $(TOP)/configs/current

MESA_LIBS := libmesa.a libmesagallium.a
DEPENDS := depend

ifeq ($(GLES_OVERLAY),1)
ES1_LIBS := libes1gallium.a
ES2_LIBS := libes2gallium.a
DEPENDS += depend.es1 depend.es2
endif

MESA_OBJ_DIR := .
ES1_OBJ_DIR := objs-es1
ES2_OBJ_DIR := objs-es2


include sources.mak

# adjust object dirs
ES1_OBJECTS := $(addprefix $(ES1_OBJ_DIR)/, $(MESA_OBJECTS))
ES2_OBJECTS := $(addprefix $(ES2_OBJ_DIR)/, $(MESA_OBJECTS))
MESA_OBJECTS := $(addprefix $(MESA_OBJ_DIR)/, $(MESA_OBJECTS))

ES1_GALLIUM_OBJECTS := $(addprefix $(ES1_OBJ_DIR)/, $(MESA_GALLIUM_OBJECTS))
ES2_GALLIUM_OBJECTS := $(addprefix $(ES2_OBJ_DIR)/, $(MESA_GALLIUM_OBJECTS))
MESA_GALLIUM_OBJECTS := $(addprefix $(MESA_OBJ_DIR)/, $(MESA_GALLIUM_OBJECTS))

# define preprocessor flags
MESA_CPPFLAGS := $(API_DEFINES) $(DEFINES)
ES1_CPPFLAGS := -DFEATURE_ES1=1 $(DEFINES)
ES2_CPPFLAGS := -DFEATURE_ES2=1 $(DEFINES)

# append include dirs
MESA_CPPFLAGS += $(INCLUDE_DIRS) $(TALLOC_CFLAGS)
ES1_CPPFLAGS += -I$(TOP)/src/mapi/es1api $(INCLUDE_DIRS) $(TALLOC_CFLAGS)
ES2_CPPFLAGS += -I$(TOP)/src/mapi/es2api $(INCLUDE_DIRS) $(TALLOC_CFLAGS)

# tidy compiler flags
CFLAGS := $(filter-out $(DEFINES), $(CFLAGS))
CXXFLAGS := $(filter-out $(DEFINES), $(CXXFLAGS))

# LLVM is needed for the state tracker
MESA_CFLAGS := $(LLVM_CFLAGS)
ES1_CFLAGS := $(LLVM_CFLAGS)
ES2_CFLAGS := $(LLVM_CFLAGS)

define mesa-cc-c
	@mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $($(1)_CPPFLAGS) $($(1)_CFLAGS) $(CFLAGS)
endef

define mesa-cxx-c
	@mkdir -p $(dir $@)
	$(CXX) -c -o $@ $< $($(1)_CPPFLAGS) $($(1)_CFLAGS) $(CXXFLAGS)
endef

$(MESA_OBJ_DIR)/%.o: %.c
	$(call mesa-cc-c,MESA)

$(MESA_OBJ_DIR)/%.o: %.cpp
	$(call mesa-cxx-c,MESA)

$(MESA_OBJ_DIR)/%.o: %.S
	$(call mesa-cc-c,MESA)

$(ES1_OBJ_DIR)/%.o: %.c
	$(call mesa-cc-c,ES1)

$(ES1_OBJ_DIR)/%.o: %.cpp
	$(call mesa-cxx-c,ES1)

$(ES1_OBJ_DIR)/%.o: %.S
	$(call mesa-cc-c,ES1)

$(ES2_OBJ_DIR)/%.o: %.c
	$(call mesa-cc-c,ES2)

$(ES2_OBJ_DIR)/%.o: %.cpp
	$(call mesa-cxx-c,ES2)

$(ES2_OBJ_DIR)/%.o: %.S
	$(call mesa-cc-c,ES2)


# Default: build dependencies, then asm_subdirs, GLSL built-in lib,
# then convenience libs (.a) and finally the device drivers:
default: $(DEPENDS) asm_subdirs \
	$(MESA_LIBS) $(ES1_LIBS) $(ES2_LIBS) driver_subdirs

main/api_exec_es1.c: main/APIspec.xml main/es_generator.py main/APIspecutil.py main/APIspec.py
	$(PYTHON2) $(PYTHON_FLAGS) main/es_generator.py -S main/APIspec.xml -V GLES1.1 > $@

main/api_exec_es2.c: main/APIspec.xml main/es_generator.py main/APIspecutil.py main/APIspec.py
	$(PYTHON2) $(PYTHON_FLAGS) main/es_generator.py -S main/APIspec.xml -V GLES2.0 > $@

######################################################################
# Helper libraries used by many drivers:

# Make archive of core mesa object files
libmesa.a: $(MESA_OBJECTS) $(GLSL_LIBS)
	@ $(MKLIB) -o mesa -static $(MESA_OBJECTS) $(GLSL_LIBS)

libes1.a: $(ES1_OBJECTS) $(GLSL_LIBS)
	@$(MKLIB) -o es1 -static $(ES1_OBJECTS) $(GLSL_LIBS)

libes2.a: $(ES2_OBJECTS) $(GLSL_LIBS)
	@$(MKLIB) -o es2 -static $(ES2_OBJECTS) $(GLSL_LIBS)

# Make archive of subset of core mesa object files for gallium
libmesagallium.a: $(MESA_GALLIUM_OBJECTS) $(GLSL_LIBS)
	@ $(MKLIB) -o mesagallium -static $(MESA_GALLIUM_OBJECTS) $(GLSL_LIBS)

libes1gallium.a: $(ES1_GALLIUM_OBJECTS) $(GLSL_LIBS)
	@$(MKLIB) -o es1gallium -static $(ES1_GALLIUM_OBJECTS) $(GLSL_LIBS)

libes2gallium.a: $(ES2_GALLIUM_OBJECTS) $(GLSL_LIBS)
	@$(MKLIB) -o es2gallium -static $(ES2_GALLIUM_OBJECTS) $(GLSL_LIBS)

######################################################################
# Device drivers
driver_subdirs: $(MESA_LIBS)
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

depend.es1: $(ALL_SOURCES)
	@echo "running $(MKDEP) for ES1"
	@touch $@
	@$(MKDEP) $(MKDEP_OPTIONS) -f$@ -p$(ES1_OBJ_DIR)/ $(ES1_CPPFLAGS) \
		$(ALL_SOURCES) > /dev/null 2>/dev/null

depend.es2: $(ALL_SOURCES)
	@echo "running $(MKDEP) for ES2"
	@touch $@
	@$(MKDEP) $(MKDEP_OPTIONS) -f$@ -p$(ES2_OBJ_DIR)/ $(ES2_CPPFLAGS) \
		$(ALL_SOURCES) > /dev/null 2>/dev/null

######################################################################
# Installation rules

# this isn't fleshed out yet but is probably the way to go in the future
new_install:
	(cd drivers && $(MAKE) install)


# XXX replace this with new_install above someday
install: default
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



# Emacs tags
tags:
	etags `find . -name \*.[ch]` $(TOP)/include/GL/*.h

clean-es1:
	-rm -f $(ES1_LIBS)
	-rm -rf $(ES1_OBJ_DIR)
	-rm -f depend.es1 depend.es1.bak

clean-es2:
	-rm -f $(ES2_LIBS)
	-rm -rf $(ES2_OBJ_DIR)
	-rm -f depend.es2 depend.es2.bak

clean: clean-es1 clean-es2
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
