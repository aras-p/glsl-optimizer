# Build the MiniGLX libGL.so library.

TOP = ../../..
include $(TOP)/configs/current


DEFINES += -DGLX_DIRECT_RENDERING -DIN_MINI_GLX -UIN_DRI_DRIVER

C_SOURCES = \
	  $(TOP)/src/mesa/main/dispatch.c \
	  $(TOP)/src/mesa/glapi/glapi.c \
	  $(TOP)/src/mesa/glapi/glthread.c \
	  $(TOP)/src/glx/x11/glcontextmodes.c \
	  miniglx.c \
	  miniglx_events.c

X86_SOURCES = $(TOP)/src/mesa/x86/glapi_x86.S

OBJECTS = $(C_SOURCES:.c=.o) \
	  $(ASM_SOURCES:.S=.o)

INCLUDES = -I. $(INCLUDE_DIRS)

INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/glx/x11 \
	-I$(TOP)/src/mesa/drivers/dri/common \
	$(LIBDRM_CFLAGS) \
	$(PCIACCESS_CFLAGS)



##### RULES #####

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES)  $< -o $@


##### TARGETS #####

default: depend $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) $(TOP)/$(LIB_DIR)/miniglx.conf


# Make libGL
$(TOP)/$(LIB_DIR)/$(GL_LIB_NAME):  $(OBJECTS) Makefile
	@ $(TOP)/bin/mklib -o $(GL_LIB) -linker '$(CC)' \
		-major 1 -minor 2 $(MKLIB_OPTIONS) \
		-install $(TOP)/$(LIB_DIR) $(GL_LIB_DEPS) $(OBJECTS) \
		$(LIBDRM_LIB) $(PCIACCESS_LIB)


# install sample miniglx.conf
$(TOP)/$(LIB_DIR)/miniglx.conf:
	$(INSTALL) example.miniglx.conf $(TOP)/$(LIB_DIR)/miniglx.conf


drmtest: xf86drm.o drmtest.o
	rm -f drmtest && $(CC) -o drmtest xf86drm.o drmtest.o


depend: $(C_SOURCES) $(ASM_SOURCES)
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(C_SOURCES) $(ASM_SOURCES) \
		> /dev/null 


# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`


# Remove .o and backup files
clean:
	-rm -f drmtest $(TOP)/$(LIB_DIR)/libGL.so*
	-rm -f *.o *~
	-rm -f depend

include depend
