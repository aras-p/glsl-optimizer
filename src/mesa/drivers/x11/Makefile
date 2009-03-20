# src/mesa/drivers/x11/Makefile for libGL.so

# This builds "stand-alone" Mesa, a version of libGL that does not need the
# GLX extension.  All rendering is converted to Xlib calls.  No hardware
# acceleration.


TOP = ../../../..

include $(TOP)/configs/current


GL_MAJOR = 1
GL_MINOR = 5
GL_TINY = 0$(MESA_MAJOR)0$(MESA_MINOR)0$(MESA_TINY)


HEADERS = \
	glxapi.h \
	glxheader.h \
	xfonts.h \
	xmesaP.h \
	xm_glide.h \
	xm_image.h

SOURCES = \
	fakeglx.c \
	glxapi.c \
	xfonts.c \
	xm_api.c \
	xm_buffer.c \
	xm_dd.c \
	xm_glide.c \
	xm_image.c \
	xm_line.c \
	xm_span.c \
	xm_tri.c

OBJECTS = $(SOURCES:.c=.o)

INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	$(X11_INCLUDES)

CORE_MESA = $(TOP)/src/mesa/libmesa.a $(TOP)/src/mesa/libglapi.a



.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@


default: $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME)


$(TOP)/$(LIB_DIR)/$(GL_LIB_NAME): $(OBJECTS) $(CORE_MESA)
	@ $(MKLIB) -o $(GL_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(GL_MAJOR) -minor $(GL_MINOR) -patch $(GL_TINY) \
		-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
		-id $(INSTALL_LIB_DIR)/lib$(GL_LIB).$(GL_MAJOR).dylib \
		$(GL_LIB_DEPS) $(OBJECTS) $(CORE_MESA)



clean:
	-rm -f *.o *~
	-rm -f depend depend.bak



depend: $(SOURCES) $(HEADERS)
	@ echo "running $(MKDEP)"
	@ touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) $(SOURCES) $(HEADERS) \
		> /dev/null 2>/dev/null


-include depend
