# src/mesa/Makefile

TOP = ../..
include $(TOP)/configs/current

include sources


GL_MAJOR = 1
GL_MINOR = 5
GL_TINY = 0$(MESA_MAJOR)0$(MESA_MINOR)0$(MESA_TINY)


.SUFFIXES : .cpp

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@

.cpp.o:
	$(CXX) -c $(INCLUDE_DIRS) $(CXXFLAGS) $< -o $@

.S.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@


# Figure out what to make here
default: depend
	@for driver in $(DRIVER_DIRS) ; do \
	  case "$$driver" in \
	    x11)      $(MAKE) xlib-driver || exit 1 ;; \
	    dri)      $(MAKE) dri-drivers || exit 1 ;; \
	    osmesa)   $(MAKE) osmesa-driver || exit 1 ;; \
	    beos)     $(MAKE) beos-driver || exit 1 ;; \
	    directfb) $(MAKE) directfb-driver || exit 1 ;; \
	    fbdev)    $(MAKE) fbdev-driver || exit 1 ;; \
	    *) echo "$$driver is invalid in DRIVER_DIRS" >&2; exit 1;; \
	  esac ; \
	done

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


######################################################################
# Helper libraries used by many drivers:

# Make archive of core mesa object files
libmesa.a: $(SOLO_OBJECTS)
	@ $(TOP)/bin/mklib -o mesa -static $(SOLO_OBJECTS);
	@if [ "${CONFIG_NAME}" = "beos" ] ; then \
		mimeset -f "$@" ; \
	fi

# Make archive of gl* API dispatcher functions only
libglapi.a: $(GLAPI_OBJECTS)
	@ $(TOP)/bin/mklib -o glapi -static $(GLAPI_OBJECTS)



######################################################################
# BeOS driver target

beos-driver: depend subdirs libmesa.a
	cd drivers/beos && $(MAKE)


######################################################################
# DRI drivers

dri-drivers: depend subdirs libmesa.a
	cd drivers/dri && $(MAKE)


######################################################################
# Xlib driver (libGL.so)

xlib-driver: depend subdirs libmesa.a libglapi.a
	cd drivers/x11 && $(MAKE)


######################################################################
# osmesa driver (libOSMesa.so)

osmesa-driver: depend subdirs libmesa.a libglapi.a
	cd drivers/osmesa && $(MAKE)


#####################################################################
# fbdev driver (libGL.so)

fbdev-driver: depend subdirs libmesa.a libglapi.a
	cd drivers/fbdev && $(MAKE)


#####################################################################
# DirectFB driver (libGL.so)

directfb-driver: depend subdirs directfb-libgl
	cd drivers/directfb && $(MAKE)


######################################################################
# Generic stuff

depend: $(ALL_SOURCES)
	@ echo "running $(MKDEP)"
	@ touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) $(ALL_SOURCES) \
		> /dev/null 2>/dev/null


subdirs:
	@ if echo "$(ASM_FLAGS)" | grep -q USE_X86_ASM ; then \
		(cd x86 && $(MAKE)) || exit 1 ; \
	fi
	@ if echo "$(ASM_FLAGS)" | grep -q USE_X86_64_ASM ; then \
		(cd x86 && $(MAKE)) || exit 1 ; \
		(cd x86-64 && $(MAKE)) || exit 1 ; \
	fi

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

install-dri:
	cd drivers/dri && $(MAKE) install

## NOT INSTALLED YET:
## $(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/include/GLES
## $(INSTALL) -m 644 include/GLES/*.h $(DESTDIR)$(INSTALL_DIR)/include/GLES


# Emacs tags
tags:
	etags `find . -name \*.[ch]` $(TOP)/include/GL/*.h

clean:
	-rm -f */*.o
	-rm -f */*/*.o
	-rm -f depend depend.bak libmesa.a libglapi.a
	-rm -f drivers/*/*.o
	-@cd drivers/dri && $(MAKE) clean
	-@cd drivers/xorg && $(MAKE) clean
	-@cd drivers/x11 && $(MAKE) clean
	-@cd drivers/osmesa && $(MAKE) clean
	-@cd x86 && $(MAKE) clean
	-@cd x86-64 && $(MAKE) clean


-include depend
