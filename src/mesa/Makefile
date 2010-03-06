# src/mesa/Makefile

TOP = ../..
include $(TOP)/configs/current

include sources.mak

.SUFFIXES : .cpp

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@

.cpp.o:
	$(CXX) -c $(INCLUDE_DIRS) $(CXXFLAGS) $< -o $@

.S.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@

CFLAGS += $(API_DEFINES)


# Default: build dependencies, then asm_subdirs, GLSL built-in lib,
# then convenience libs (.a) and finally the device drivers:
default: depend asm_subdirs glsl_builtin libmesa.a libmesagallium.a \
	libglapi.a driver_subdirs


main/api_exec_es1.c: main/APIspec.xml main/es_generator.py main/APIspecutil.py main/APIspec.py
	$(PYTHON2) $(PYTHON_FLAGS) main/es_generator.py -S main/APIspec.xml -V GLES1.1 > $@

main/api_exec_es2.c: main/APIspec.xml main/es_generator.py main/APIspecutil.py main/APIspec.py
	$(PYTHON2) $(PYTHON_FLAGS) main/es_generator.py -S main/APIspec.xml -V GLES2.0 > $@

main/get_es1.c: main/get_gen_es.py
	$(PYTHON2) $(PYTHON_FLAGS) $< 1 > $@

main/get_es2.c: main/get_gen_es.py
	$(PYTHON2) $(PYTHON_FLAGS) $< 2 > $@

######################################################################
# Helper libraries used by many drivers:

# Make archive of core mesa object files
libmesa.a: $(MESA_OBJECTS) $(GLSL_LIBS)
	@ $(MKLIB) -o mesa -static $(MESA_OBJECTS) $(GLSL_LIBS)

# Make archive of subset of core mesa object files for gallium
libmesagallium.a: $(MESA_GALLIUM_OBJECTS) $(GLSL_LIBS)
	@ $(MKLIB) -o mesagallium -static $(MESA_GALLIUM_OBJECTS) $(GLSL_LIBS)

# Make archive of gl* API dispatcher functions only
libglapi.a: $(GLAPI_OBJECTS)
	@ $(MKLIB) -o glapi -static $(GLAPI_OBJECTS)

######################################################################
# Device drivers
driver_subdirs: libmesa.a libglapi.a libmesagallium.a
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
# GLSL built-in library
glsl_builtin:
	(cd shader/slang/library && $(MAKE)) || exit 1 ;


######################################################################
# Dependency generation

depend: $(ALL_SOURCES)
	@ echo "running $(MKDEP)"
	@ touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) $(ALL_SOURCES) \
		> /dev/null 2>/dev/null


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


gl_pcedit = sed \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),' \
	-e 's,@INSTALL_LIB_DIR@,$(INSTALL_LIB_DIR),' \
	-e 's,@INSTALL_INC_DIR@,$(INSTALL_INC_DIR),' \
	-e 's,@VERSION@,$(MESA_MAJOR).$(MESA_MINOR).$(MESA_TINY),' \
	-e 's,@GL_PC_REQ_PRIV@,$(GL_PC_REQ_PRIV),' \
	-e 's,@GL_PC_LIB_PRIV@,$(GL_PC_LIB_PRIV),' \
	-e 's,@GL_PC_CFLAGS@,$(GL_PC_CFLAGS),' \
	-e 's,@GL_LIB@,$(GL_LIB),'

gl.pc: gl.pc.in
	$(gl_pcedit) $< > $@

osmesa_pcedit = sed \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),' \
	-e 's,@INSTALL_LIB_DIR@,$(INSTALL_LIB_DIR),' \
	-e 's,@INSTALL_INC_DIR@,$(INSTALL_INC_DIR),' \
	-e 's,@VERSION@,$(MESA_MAJOR).$(MESA_MINOR).$(MESA_TINY),' \
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

install-headers-es1:
	$(INSTALL) -d $(DESTDIR)$(INSTALL_INC_DIR)/GLES
	$(INSTALL) -m 644 $(TOP)/include/GLES/*.h \
		$(DESTDIR)$(INSTALL_INC_DIR)/GLES

install-es1: default glesv1_cm.pc install-headers-es1
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig
	$(MINSTALL) $(TOP)/$(LIB_DIR)/$(GLESv1_CM_LIB_GLOB) \
		$(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -m 644 glesv1_cm.pc $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig

install-headers-es2:
	$(INSTALL) -d $(DESTDIR)$(INSTALL_INC_DIR)/GLES2
	$(INSTALL) -m 644 $(TOP)/include/GLES2/*.h \
		$(DESTDIR)$(INSTALL_INC_DIR)/GLES2

install-es2: default glesv2.pc install-headers-es2
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig
	$(MINSTALL) $(TOP)/$(LIB_DIR)/$(GLESv2_LIB_GLOB) \
		$(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -m 644 glesv2.pc $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig

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


clean:
	-rm -f */*.o
	-rm -f */*/*.o
	-rm -f depend depend.bak libmesa.a libglapi.a libmesagallium.a
	-rm -f drivers/*/*.o
	-rm -f *.pc
	-rm -f shader/slang/library/*_gc.h
	-@cd drivers/dri && $(MAKE) clean
	-@cd drivers/x11 && $(MAKE) clean
	-@cd drivers/osmesa && $(MAKE) clean
	-@cd x86 && $(MAKE) clean
	-@cd x86-64 && $(MAKE) clean


-include depend
