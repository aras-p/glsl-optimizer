TOP = ../../../..
include $(TOP)/configs/current

common_INCLUDES = \
	-I. \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/egl/main \
	-I$(TOP)/include

common_SOURCES = $(wildcard common/*.c)
common_OBJECTS = $(common_SOURCES:.c=.o)


x11_INCLUDES = \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/glx \
	-I$(TOP)/src/mesa \
	$(X11_CFLAGS) \
	$(shell pkg-config --cflags-only-I libdrm)

x11_SOURCES = $(wildcard x11/*.c) \
	      $(TOP)/src/glx/dri2.c
x11_OBJECTS = $(x11_SOURCES:.c=.o)


kms_INCLUDES = $(shell pkg-config --cflags-only-I libdrm)
kms_SOURCES = $(wildcard kms/*.c)
kms_OBJECTS = $(kms_SOURCES:.c=.o)


ALL_INCLUDES = $(common_INCLUDES) $(x11_INCLUDES) $(kms_INCLUDES)
ALL_SOURCES = $(common_SOURCES) $(x11_SOURCES) $(kms_SOURCES)
ALL_OBJECTS = $(common_OBJECTS) $(x11_OBJECTS) $(kms_OBJECTS)

##### TARGETS #####

EGL_DISPLAYS_MODS = $(foreach dpy, $(EGL_DISPLAYS), libegl$(dpy).a)

default: depend $(EGL_DISPLAYS_MODS)


libeglx11.a: $(x11_OBJECTS) $(common_OBJECTS) Makefile
	$(MKLIB) -o eglx11 -static $(x11_OBJECTS) $(common_OBJECTS)

libeglkms.a: $(kms_OBJECTS) $(common_OBJECTS) Makefile
	$(MKLIB) -o eglkms -static $(kms_OBJECTS) $(common_OBJECTS)

depend: 
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(ALL_INCLUDES) $(ALL_SOURCES) 2> /dev/null

clean:
	rm -f $(ALL_OBJECTS)
	rm -f $(EGL_DISPLAYS_MODS)
	rm -f depend depend.bak

# Dummy target
install:
	@echo -n ""

##### RULES #####

$(common_OBJECTS): %.o: %.c
	$(CC) -c $(common_INCLUDES) $(DEFINES) $(CFLAGS) $< -o $@

$(x11_OBJECTS): %.o: %.c
	$(CC) -c $(common_INCLUDES) $(x11_INCLUDES) $(DEFINES) $(CFLAGS) $< -o $@

$(kms_OBJECTS): %.o: %.c
	$(CC) -c $(common_INCLUDES) $(kms_INCLUDES) $(DEFINES) $(CFLAGS) $< -o $@

sinclude depend
