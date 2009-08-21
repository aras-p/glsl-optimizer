# src/egl/main/Makefile

TOP = ../../..
include $(TOP)/configs/current


INCLUDE_DIRS = -I$(TOP)/include -I$(TOP)/src/mesa/glapi $(X11_INCLUDES)

HEADERS = \
	eglcompiler.h \
	eglconfig.h \
	eglconfigutil.h \
	eglcontext.h \
	eglcurrent.h \
	egldefines.h \
	egldisplay.h \
	egldriver.h \
	eglglobals.h \
	egllog.h \
	eglmisc.h \
	eglmode.h \
	eglmutex.h \
	eglscreen.h \
	eglstring.h \
	eglsurface.h

SOURCES = \
	eglapi.c \
	eglconfig.c \
	eglconfigutil.c \
	eglcontext.c \
	eglcurrent.c \
	egldisplay.c \
	egldriver.c \
	eglglobals.c \
	egllog.c \
	eglmisc.c \
	eglmode.c \
	eglscreen.c \
	eglstring.c \
	eglsurface.c

OBJECTS = $(SOURCES:.c=.o)


# Undefined for now
LOCAL_CFLAGS = -D_EGL_PLATFORM_X=1


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(LOCAL_CFLAGS) $< -o $@



default: depend library


# EGL Library
library: $(TOP)/$(LIB_DIR)/libEGL.so

$(TOP)/$(LIB_DIR)/libEGL.so: $(OBJECTS)
	$(MKLIB) -o EGL -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major 1 -minor 0 \
		-install $(TOP)/$(LIB_DIR) \
		$(EGL_LIB_DEPS) $(OBJECTS)

install: default
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(MINSTALL) $(TOP)/$(LIB_DIR)/libEGL.so* $(DESTDIR)$(INSTALL_LIB_DIR)

clean:
	-rm -f *.o *.so*
	-rm -f core.*
	-rm -f depend depend.bak


depend: $(SOURCES) $(HEADERS)
	@ echo "running $(MKDEP)"
	@ rm -f depend
	@ touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) \
		$(SOURCES) $(HEADERS) > /dev/null 2>/dev/null


include depend
# DO NOT DELETE
