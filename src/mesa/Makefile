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
default:
	@for driver in $(DRIVER_DIRS) ; do \
	  case "$$driver" in \
	    x11)      $(MAKE) stand-alone ;; \
	    dri)      $(MAKE) linux-solo ;; \
	    osmesa)   $(MAKE) osmesa-only ;; \
	    beos)     $(MAKE) beos ;; \
	    directfb) $(MAKE) directfb ;; \
	    fbdev)    $(MAKE) fbdev ;; \
	    *) echo "$$driver is invalid in DRIVER_DIRS" >&2; exit 1;; \
	  esac ; \
	done


######################################################################
# BeOS driver target

beos: depend subdirs libmesa.a
	cd drivers/beos && $(MAKE)


######################################################################
# Linux DRI drivers

# Make archive of core object files
libmesa.a: $(SOLO_OBJECTS)
	@ $(TOP)/bin/mklib -o mesa -static $(SOLO_OBJECTS);
	@if [ "${CONFIG_NAME}" = "beos" ] ; then \
		mimeset -f "$@" ; \
	fi

linux-solo: depend subdirs libmesa.a
	cd drivers/dri && $(MAKE)


#####################################################################
# Stand-alone Mesa libGL, no built-in drivers (DirectFB)

libgl-core: $(CORE_OBJECTS)
	@ $(TOP)/bin/mklib -o $(GL_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(MESA_MAJOR) -minor $(MESA_MINOR) -patch $(MESA_TINY) \
		-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) $(CORE_OBJECTS) \
		$(GL_LIB_DEPS)

directfb: depend subdirs libgl-core
	cd drivers/directfb && $(MAKE)


#####################################################################
# fbdev Mesa driver (libGL.so)

fbdev: $(CORE_OBJECTS) $(FBDEV_DRIVER_OBJECTS) $(COMMON_DRIVER_OBJECTS)
	@ $(TOP)/bin/mklib -o $(GL_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(MESA_MAJOR) -minor $(MESA_MINOR) -patch $(MESA_TINY) \
		-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
		$(CORE_OBJECTS) $(FBDEV_DRIVER_OBJECTS) \
		$(COMMON_DRIVER_OBJECTS) $(GL_LIB_DEPS)


######################################################################
# Stand-alone Mesa libGL and libOSMesa
STAND_ALONE_DRIVER_SOURCES = \
	$(COMMON_DRIVER_SOURCES) \
	$(X11_DRIVER_SOURCES)

STAND_ALONE_DRIVER_OBJECTS = $(STAND_ALONE_DRIVER_SOURCES:.c=.o)

STAND_ALONE_OBJECTS = \
	$(CORE_OBJECTS) \
	$(STAND_ALONE_DRIVER_OBJECTS)

# For libOSMesa16 or libOSMesa32 we link _all_ the objects into the library,
# not just the osmesa.o object (i.e. we don't have a libGL).
OSMESA16_OBJECTS = \
	$(CORE_OBJECTS) \
	$(COMMON_DRIVER_OBJECTS) \
	$(OSMESA_DRIVER_OBJECTS)


stand-alone: depend subdirs libmesa.a \
	$(TOP)/$(LIB_DIR)/$(GL_LIB_NAME)

osmesa-only: depend subdirs \
	$(TOP)/$(LIB_DIR)/$(OSMESA_LIB_NAME)

# Make the GL library
$(TOP)/$(LIB_DIR)/$(GL_LIB_NAME): $(STAND_ALONE_OBJECTS)
	@ $(TOP)/bin/mklib -o $(GL_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(GL_MAJOR) -minor $(GL_MINOR) -patch $(GL_TINY) \
		-install $(TOP)/$(LIB_DIR) \
		$(MKLIB_OPTIONS) $(GL_LIB_DEPS) $(STAND_ALONE_OBJECTS)

# Make the OSMesa library
$(TOP)/$(LIB_DIR)/$(OSMESA_LIB_NAME): $(OSMESA_DRIVER_OBJECTS) $(OSMESA16_OBJECTS)
	@ if [ "${DRIVER_DIRS}" = "osmesa" ] ; then \
		$(TOP)/bin/mklib -o $(OSMESA_LIB) -linker '$(CC)' \
			-ldflags '$(LDFLAGS)' -major $(MESA_MAJOR) \
			-minor $(MESA_MINOR) -patch $(MESA_TINY) \
			-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
			$(OSMESA_LIB_DEPS) $(OSMESA16_OBJECTS) ; \
	else \
		$(TOP)/bin/mklib -o $(OSMESA_LIB) -linker '$(CC)' \
			-ldflags '$(LDFLAGS)' -major $(MESA_MAJOR) \
			-minor $(MESA_MINOR) -patch $(GL_TINY) \
			-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
			$(OSMESA_LIB_DEPS) $(OSMESA_DRIVER_OBJECTS) ; \
	fi


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

install-libgl: gl.pc
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/include/GL
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)/pkgconfig
	$(INSTALL) -m 644 $(TOP)/include/GL/*.h $(DESTDIR)$(INSTALL_DIR)/include/GL
	@if [ -e $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) ]; then \
		$(INSTALL) $(TOP)/$(LIB_DIR)/libGL* \
			$(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR); \
	fi
	$(INSTALL) -m 644 gl.pc $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)/pkgconfig

install-osmesa:
	@if [ -e $(TOP)/$(LIB_DIR)/$(OSMESA_LIB_NAME) ]; then \
		$(INSTALL) $(TOP)/$(LIB_DIR)/libOSMesa* \
			$(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR); \
	fi

install-drivers:
	@for target in $(DRIVER_DIRS); do \
	  case "$$target" in \
	    dri) (cd drivers/dri && $(MAKE) install) || exit 1 ;; \
	  esac; \
	done

install: default install-libgl install-osmesa install-drivers

## NOT INSTALLED YET:
## $(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/include/GLES
## $(INSTALL) -m 644 include/GLES/*.h $(DESTDIR)$(INSTALL_DIR)/include/GLES


# Emacs tags
tags:
	etags `find . -name \*.[ch]` $(TOP)/include/GL/*.h

clean:
	-rm -f */*.o
	-rm -f */*/*.o
	-rm -f depend depend.bak libmesa.a
	-rm -f drivers/*/*.o
	(cd drivers/dri && $(MAKE) clean)
	(cd x86 && $(MAKE) clean)
	(cd x86-64 && $(MAKE) clean)


-include depend
