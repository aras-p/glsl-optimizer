# progs/egl/opengles2/Makefile

TOP = ../../..
include $(TOP)/configs/current


INCLUDE_DIRS = \
	-I$(TOP)/include \
	$(X11_CFLAGS)

HEADERS = $(TOP)/include/GLES/egl.h


ES2_LIB_DEPS = \
	$(TOP)/$(LIB_DIR)/libEGL.so \
	$(TOP)/$(LIB_DIR)/libGLESv2.so


ES2_LIBS = \
	-L$(TOP)/$(LIB_DIR) -lEGL \
	-L$(TOP)/$(LIB_DIR) -lGLESv2 $(LIBDRM_LIB) $(X11_LIBS)

PROGRAMS = \
	es2_info \
	tri


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: $(PROGRAMS)



es2_info.c: ../opengles1/es1_info.c
	cp -f $^ $@

es2_info: es2_info.o $(ES2_LIB_DEPS)
	$(CC) $(CFLAGS) es2_info.o $(ES2_LIBS) -o $@

tri: tri.o $(ES2_LIB_DEPS)
	$(CC) $(CFLAGS) tri.o $(ES2_LIBS) -o $@



clean:
	rm -f *.o *~
	rm -f $(PROGRAMS)
	rm -f es2_info.c

