# src/gallium/winsys/xlib/Makefile

# This makefile produces a "stand-alone" libGL.so which is based on
# Xlib (no DRI HW acceleration)


TOP = ../../../../../..
include $(TOP)/configs/current


GL_MAJOR = 1
GL_MINOR = 5
GL_TINY = 0$(MESA_MAJOR)0$(MESA_MINOR)0$(MESA_TINY)


INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/gallium/drivers/i965 \
	-I$(TOP)/src/gallium/drivers/i965/include \
	-I$(TOP)/src/gallium/state_trackers/glx/xlib \
	-I$(TOP)/src/gallium/auxiliary \
	-I/usr/include/drm

XLIB_WINSYS_SOURCES = \
	xlib_i965.c \



XLIB_WINSYS_OBJECTS = $(XLIB_WINSYS_SOURCES:.c=.o)



LIBS = \
	$(TOP)/src/gallium/drivers/i965/libi965.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/state_trackers/glx/xlib/libxlib.a \
	$(TOP)/src/mesa/libglapi.a \
	$(TOP)/src/mesa/libmesagallium.a \
	$(GALLIUM_AUXILIARIES) 

#	$(TOP)/src/gallium/drivers/i965/lib/libi9xx.a \

.SUFFIXES : .cpp

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(DEFINES) $(CFLAGS) $< -o $@

.cpp.o:
	$(CXX) -c $(INCLUDE_DIRS) $(DEFINES) $(CXXFLAGS) $< -o $@



default: $(TOP)/$(LIB_DIR)/gallium $(TOP)/$(LIB_DIR)/gallium/$(GL_LIB_NAME)

$(TOP)/$(LIB_DIR)/gallium:
	@ mkdir -p $(TOP)/$(LIB_DIR)/gallium

# Make the libGL.so library
$(TOP)/$(LIB_DIR)/gallium/$(GL_LIB_NAME): $(XLIB_WINSYS_OBJECTS) $(LIBS) Makefile
	$(TOP)/bin/mklib -o $(GL_LIB) \
		-linker "$(CC)" \
		-major $(GL_MAJOR) -minor $(GL_MINOR) -patch $(GL_TINY) \
		-install $(TOP)/$(LIB_DIR)/gallium \
		$(MKLIB_OPTIONS) $(XLIB_WINSYS_OBJECTS) \
		-Wl,--start-group $(LIBS) -Wl,--end-group $(GL_LIB_DEPS)


depend: $(XLIB_WINSYS_SOURCES)
	@ echo "running $(MKDEP)"
	@ rm -f depend  # workaround oops on gutsy?!?
	@ touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) $(XLIB_WINSYS_SOURCES) \
		> /dev/null 2>/dev/null


install: default
	$(INSTALL) -d $(INSTALL_DIR)/include/GL
	$(INSTALL) -d $(INSTALL_DIR)/$(LIB_DIR)
	$(INSTALL) -m 644 $(TOP)/include/GL/*.h $(INSTALL_DIR)/include/GL
	@if [ -e $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) ]; then \
		$(MINSTALL) $(TOP)/$(LIB_DIR)/libGL* $(INSTALL_DIR)/$(LIB_DIR); \
	fi


# Emacs tags
tags:
	etags `find . -name \*.[ch]` $(TOP)/include/GL/*.h

clean:
	-rm -f *.o


include depend
