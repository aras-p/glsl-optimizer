# Build a subset DRI-based libGL.so library.
# Indirect rendering not supported, etc.

TOP = ../../..
include $(TOP)/configs/current


C_SOURCES = dispatch.c \
	  dri_util.c \
	  ../../mesa/glapi/glapi.c \
	  ../../mesa/glapi/glthread.c \
	  miniglx.c \
	  miniglx_events.c \
	  xf86drm.c 

OBJECTS = $(C_SOURCES:.c=.o)

INCLUDES = -I. $(INCLUDE_DIRS)

INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa/math \
	-I$(TOP)/src/mesa/transform \
	-I$(TOP)/src/mesa/swrast \
	-I$(TOP)/src/mesa/swrast_setup \
	-I$(DRM_SOURCE_PATH)/shared


##### RULES #####

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES)  $< -o $@


##### TARGETS #####

default: depend $(LIB_DIR)/$(GL_LIB_NAME)


# Make libGL
$(LIB_DIR)/$(GL_LIB_NAME):  $(OBJECTS) Makefile
	CC=$(CC) CXX=$(CXX) $(TOP)/bin/mklib -o $(GL_LIB) -major 1 -minor 2 $(MKLIB_OPTIONS) \
		-install $(LIB_DIR) $(GL_LIB_DEPS) $(OBJECTS)
	rm -f $(LIB_DIR)/miniglx.conf
	install example.miniglx.conf $(LIB_DIR)/miniglx.conf


drmtest: xf86drm.o drmtest.o
	rm -f drmtest && $(CC) -o drmtest xf86drm.o drmtest.o


depend: $(C_SOURCES) $(ASM_SOURCES)
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(C_SOURCES) $(ASM_SOURCES) \
		> /dev/null 2>&1


# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`


# Remove .o and backup files
clean:
	-rm -f drmtest $(LIB_DIR)/libGL.so*
	-rm -f *.o *~
	-rm -f depend

include depend
