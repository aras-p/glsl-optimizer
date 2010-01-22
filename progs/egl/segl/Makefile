# progs/egl/segl/Makefile

TOP = ../../..
include $(TOP)/configs/current

SEGL_LIBS := $(foreach dpy, $(EGL_DISPLAYS), libsegl-$(dpy).a)

all: $(SEGL_LIBS)

x11_OBJECTS := segl.o segl_x11.o
kms_OBJECTS := segl.o segl_kms.o

libsegl-x11.a: $(x11_OBJECTS)
	$(MKLIB) -o segl-x11 -static $(x11_OBJECTS)

libsegl-kms.a: $(kms_OBJECTS)
	$(MKLIB) -o segl-kms -static $(kms_OBJECTS)

clean:
	rm -f $(sort $(x11_OBJECTS) $(kms_OBJECTS))
	rm -f $(SEGL_LIBS)
