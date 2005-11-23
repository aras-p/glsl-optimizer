# src/egl/main/Makefile

TOP = ../../..
include $(TOP)/configs/current


INCLUDE_DIRS = -I$(TOP)/include -I$(TOP)/src/mesa/glapi

HEADERS = \
	eglconfig.h \
	eglcontext.h \
	egldisplay.h \
	egldriver.h \
	eglglobals.h \
	egllog.h \
	eglhash.h \
	eglmode.h \
	eglscreen.h \
	eglsurface.h

SOURCES = \
	eglapi.c \
	eglconfig.c \
	eglcontext.c \
	egldisplay.c \
	egldriver.c \
	eglglobals.c \
	egllog.c \
	eglhash.c \
	eglmode.c \
	eglscreen.c \
	eglsurface.c

OBJECTS = $(SOURCES:.c=.o)


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: depend library


# EGL Library
library: $(LIB_DIR)/libEGL.so

$(LIB_DIR)/libEGL.so: $(OBJECTS)
	$(TOP)/bin/mklib -o EGL -major 1 -minor 0 \
		-install $(LIB_DIR) -ldl $(OBJECTS)



clean:
	rm -f *.o *.so*
	rm -f core.*


depend: $(SOURCES) $(HEADERS)
	@ echo "running $(MKDEP)"
	@ touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) \
		$(SOURCES) $(HEADERS) > /dev/null 

include depend
# DO NOT DELETE
