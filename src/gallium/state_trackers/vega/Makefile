# src/gallium/state_trackers/vega/Makefile

TOP = ../../../..
include $(TOP)/configs/current

VG_LIB = OpenVG
VG_LIB_NAME = lib$(VG_LIB).so

VG_MAJOR = 1
VG_MINOR = 0
VG_TINY = 0

### Lists of source files, included by Makefiles

VG_SOURCES = \
           api_context.c   \
           api_filters.c   \
           api_images.c    \
           api_masks.c     \
           api_misc.c      \
           api_paint.c     \
           api_params.c    \
           api_path.c      \
           api_text.c      \
           api_transform.c \
           vgu.c        \
           vg_context.c \
           vg_state.c   \
           vg_tracker.c \
           vg_translate.c \
           polygon.c    \
           bezier.c     \
           path.c       \
           paint.c      \
           arc.c \
           image.c \
           renderer.c \
           stroker.c \
           mask.c \
           shader.c \
           shaders_cache.c

VG_OBJECTS = $(VG_SOURCES:.c=.o)

VG_LIBS = $(GALLIUM_AUXILIARIES)
VG_LIB_DEPS = $(EXTRA_LIB_PATH) -lm

### Include directories

INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/auxiliary


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(DEFINES) $(CFLAGS) $< -o $@

default: depend $(TOP)/$(LIB_DIR)/$(VG_LIB_NAME)

# Make the OpenVG library
$(TOP)/$(LIB_DIR)/$(VG_LIB_NAME): $(VG_OBJECTS) $(VG_LIBS)
	$(MKLIB) -o $(VG_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(VG_MAJOR) \
		-minor $(VG_MINOR) \
		-patch $(VG_TINY) \
		-install $(TOP)/$(LIB_DIR) \
		$(VG_OBJECTS) $(VG_LIBS) $(VG_LIB_DEPS)

######################################################################
# Generic stuff

depend: $(VG_SOURCES)
	@ echo "running $(MKDEP)"
	@ rm -f depend  # workaround oops on gutsy?!?
	@ touch depend
	@ $(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) $(VG_SOURCES) \
		> /dev/null 2>/dev/null

install: default
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/include/VG
	$(INSTALL) -m 644 $(TOP)/include/VG/*.h $(DESTDIR)$(INSTALL_DIR)/include/VG
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)
	$(MINSTALL) $(TOP)/$(LIB_DIR)/libOpenVG* $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)

# Emacs tags
tags:
	etags `find . -name \*.[ch]` $(TOP)/include/VG/*.h

clean:
	rm -f $(VG_OBJECTS)
	rm -f depend depend.bak

sinclude depend
