# src/egl/main/Makefile

TOP = ../../..
include $(TOP)/configs/current


INCLUDE_DIRS = -I$(TOP)/include -I$(TOP)/src/mesa/glapi

HEADERS = \
	eglconfig.h \
	eglconfigutil.h \
	eglcontext.h \
	egldefines.h \
	egldisplay.h \
	egldriver.h \
	eglglobals.h \
	egllog.h \
	eglhash.h \
	eglmisc.h \
	eglmode.h \
	eglscreen.h \
	eglstring.h \
	eglsurface.h \
	eglx.h

SOURCES = \
	eglapi.c \
	eglconfig.c \
	eglconfigutil.c \
	eglcontext.c \
	egldisplay.c \
	egldriver.c \
	eglglobals.c \
	egllog.c \
	eglhash.c \
	eglmisc.c \
	eglmode.c \
	eglscreen.c \
	eglstring.c \
	eglsurface.c \
	eglx.c

OBJECTS = $(SOURCES:.c=.o)


# Undefined for now
LOCAL_CFLAGS = -D_EGL_PLATFORM_X=1

LIBS = -lX11


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(LOCAL_CFLAGS) $< -o $@



default: depend library


# EGL Library
library: $(TOP)/$(LIB_DIR)/libEGL.so

$(TOP)/$(LIB_DIR)/libEGL.so: $(OBJECTS)
	$(TOP)/bin/mklib -o EGL -major 1 -minor 0 \
		-install $(TOP)/$(LIB_DIR) -ldl $(OBJECTS) $(LIBS)



clean:
	rm -f *.o *.so*
	rm -f core.*


depend: $(SOURCES) $(HEADERS)
	@ echo "running $(MKDEP)"
	@ rm -f depend
	@ touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) \
		$(SOURCES) $(HEADERS) > /dev/null 2>/dev/null


include depend
# DO NOT DELETE
