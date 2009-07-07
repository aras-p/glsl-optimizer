# src/mesa/Makefile

TOP = ../../../..
include $(TOP)/configs/current
GALLIUM = $(TOP)

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


### All the core C sources

ALL_SOURCES = \
        $(VG_SOURCES)


### Object files
VG_OBJECTS = \
	$(VG_SOURCES:.c=.o)

### Include directories

INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(GALLIUM)/include \
	-I$(GALLIUM)/src/gallium/include \
	-I$(GALLIUM)/src/gallium/auxiliary

VG_LIB = OpenVG
VG_LIB_NAME = lib$(VG_LIB).so

VG_MAJOR = 1
VG_MINOR = 0
VG_TINY = 0

GALLIUM_LIBS = \
	$(GALLIUM)/src/gallium/auxiliary/pipebuffer/libpipebuffer.a \
	$(GALLIUM)/src/gallium/auxiliary/sct/libsct.a \
	$(GALLIUM)/src/gallium/auxiliary/draw/libdraw.a \
	$(GALLIUM)/src/gallium/auxiliary/rtasm/librtasm.a \
	$(GALLIUM)/src/gallium/auxiliary/translate/libtranslate.a \
	$(GALLIUM)/src/gallium/auxiliary/cso_cache/libcso_cache.a \
	$(GALLIUM)/src/gallium/auxiliary/util/libutil.a \
	$(GALLIUM)/src/gallium/auxiliary/tgsi/libtgsi.a

.SUFFIXES : .cpp

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@

.cpp.o:
	$(CXX) -c $(INCLUDE_DIRS) $(CXXFLAGS) $< -o $@

.S.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@


default: depend subdirs $(TOP)/$(LIB_DIR)/$(VG_LIB_NAME)

# Make the OpenVG library
$(TOP)/$(LIB_DIR)/$(VG_LIB_NAME): $(VG_OBJECTS) $(GALLIUM_LIBS)
	$(TOP)/bin/mklib -o $(VG_LIB) \
		-major $(VG_MAJOR) \
		-minor $(VG_MINOR) \
		-patch $(VG_TINY) \
		-install $(TOP)/$(LIB_DIR) \
		$(VG_OBJECTS) $(GALLIUM_LIBS) \
		-Wl,--whole-archive $(LIBS) -Wl,--no-whole-archive $(SYS_LIBS)

######################################################################
# Generic stuff

depend: $(ALL_SOURCES)
	@ echo "running $(MKDEP)"
	@ rm -f depend  # workaround oops on gutsy?!?
	@ touch depend
	@ $(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) $(ALL_SOURCES) \
		> /dev/null 2>/dev/null


subdirs:

install: default
	$(INSTALL) -d $(INSTALL_DIR)/include/VG
	$(INSTALL) -d $(INSTALL_DIR)/$(LIB_DIR)
	$(INSTALL) -m 644 $(TOP)/include/VG/*.h $(INSTALL_DIR)/include/VG
	@if [ -e $(TOP)/$(LIB_DIR)/$(VG_LIB_NAME) ]; then \
		$(INSTALL) $(TOP)/$(LIB_DIR)/libOpenVG* $(INSTALL_DIR)/$(LIB_DIR); \
	fi

# Emacs tags
tags:
	etags `find . -name \*.[ch]` $(TOP)/include/VG/*.h

clean:
	-rm -f *.o
	-rm -f */*.o
	-rm -f */*/*.o
	-rm -f depend depend.bak

include depend
