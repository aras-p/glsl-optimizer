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



GLAPI_LIB = libglapi.a


# Default: build dependencies, then asm_subdirs, then convenience
# libs (.a) and finally the device drivers:
default: depend asm_subdirs libmesa.a $(GLAPI_LIB) driver_subdirs



######################################################################
# Helper libraries used by many drivers:

# Make archive of core mesa object files
libmesa.a: $(MESA_OBJECTS)
	@ $(TOP)/bin/mklib -o mesa -static $(MESA_OBJECTS)

# Make archive of gl* API dispatcher functions only
$(GLAPI_LIB): $(GLAPI_OBJECTS)
	@if [ "${WINDOW_SYSTEM}" = "dri" ] ; then \
		touch libglapi.a ; \
	else \
		$(TOP)/bin/mklib -o glapi -static $(GLAPI_OBJECTS) ; \
	fi


######################################################################
# Device drivers
driver_subdirs: libmesa.a $(GLAPI_LIB)
	(cd drivers && $(MAKE))


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


pcedit = sed \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),' \
	-e 's,@LIB_DIR@,$(LIB_DIR),' \
	-e 's,@VERSION@,$(MESA_MAJOR).$(MESA_MINOR).$(MESA_TINY),'

gl.pc: gl.pc.in
	$(pcedit) $< > $@

install-headers:
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/include/GL
	$(INSTALL) -m 644 $(TOP)/include/GL/*.h \
		$(DESTDIR)$(INSTALL_DIR)/include/GL

install-libgl: default gl.pc install-headers
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)/pkgconfig
	$(INSTALL) $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME)* \
		$(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)
	$(INSTALL) -m 644 gl.pc $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)/pkgconfig

install-osmesa: default
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)
	$(INSTALL) $(TOP)/$(LIB_DIR)/$(OSMESA_LIB_NAME)* \
		$(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)

install-dri: default
	cd drivers/dri && $(MAKE) install



# Emacs tags
tags:
	etags `find . -name \*.[ch]` $(TOP)/include/GL/*.h


clean:
	-rm -f */*.o
	-rm -f */*/*.o
	-rm -f depend depend.bak libmesa.a $(GLAPI_LIB)
	-rm -f drivers/*/*.o
	-@cd drivers/dri && $(MAKE) clean
	-@cd drivers/xorg && $(MAKE) clean
	-@cd drivers/x11 && $(MAKE) clean
	-@cd drivers/osmesa && $(MAKE) clean
	-@cd x86 && $(MAKE) clean
	-@cd x86-64 && $(MAKE) clean


-include depend
