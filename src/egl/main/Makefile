# src/egl/main/Makefile

TOP = ../../..
include $(TOP)/configs/current


EGL_MAJOR = 1
EGL_MINOR = 0

INCLUDE_DIRS = -I$(TOP)/include $(X11_CFLAGS)

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
	eglimage.h \
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
	eglimage.c \
	egllog.c \
	eglmisc.c \
	eglmode.c \
	eglscreen.c \
	eglstring.c \
	eglsurface.c

OBJECTS = $(SOURCES:.c=.o)


# use dl*() to load drivers
LOCAL_CFLAGS = -D_EGL_PLATFORM_POSIX=1

EGL_DEFAULT_DISPLAY = $(word 1, $(EGL_DISPLAYS))

LOCAL_CFLAGS += \
	-D_EGL_DEFAULT_DISPLAY=\"$(EGL_DEFAULT_DISPLAY)\" \
	-D_EGL_DRIVER_SEARCH_DIR=\"$(EGL_DRIVER_INSTALL_DIR)\"

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(LOCAL_CFLAGS) $< -o $@



default: depend library


# EGL Library
library: $(TOP)/$(LIB_DIR)/$(EGL_LIB_NAME)

$(TOP)/$(LIB_DIR)/$(EGL_LIB_NAME): $(OBJECTS)
	$(MKLIB) -o $(EGL_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(EGL_MAJOR) -minor $(EGL_MINOR) \
		-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
		$(EGL_LIB_DEPS) $(OBJECTS)

install-headers:
	$(INSTALL) -d $(DESTDIR)$(INSTALL_INC_DIR)/KHR
	$(INSTALL) -m 644 $(TOP)/include/KHR/*.h \
		$(DESTDIR)$(INSTALL_INC_DIR)/KHR
	$(INSTALL) -d $(DESTDIR)$(INSTALL_INC_DIR)/EGL
	$(INSTALL) -m 644 $(TOP)/include/EGL/*.h \
		$(DESTDIR)$(INSTALL_INC_DIR)/EGL

install: default install-headers
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(MINSTALL) $(TOP)/$(LIB_DIR)/$(EGL_LIB_GLOB) \
		$(DESTDIR)$(INSTALL_LIB_DIR)

clean:
	-rm -f *.o
	-rm -f depend depend.bak


depend: $(SOURCES) $(HEADERS)
	@ echo "running $(MKDEP)"
	@ rm -f depend
	@ touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) \
		$(SOURCES) $(HEADERS) > /dev/null 2>/dev/null


-include depend
# DO NOT DELETE
