# progs/egl/opengles2/Makefile

TOP = ../../..
include $(TOP)/configs/current


INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/progs/egl/eglut \
	$(X11_CFLAGS) \


HEADERS = $(TOP)/include/GLES/egl.h


ES2_LIB_DEPS = \
	$(TOP)/$(LIB_DIR)/$(EGL_LIB_NAME) \
	$(TOP)/$(LIB_DIR)/$(GLESv2_LIB_NAME)


ES2_LIBS = \
	-L$(TOP)/$(LIB_DIR) -l$(EGL_LIB) -l$(GLESv2_LIB) \
	$(LIBDRM_LIB) $(X11_LIBS)

PROGRAMS = \
	es2_info \
	tri \
	es2gears


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: $(PROGRAMS)



es2_info.c: ../opengles1/es1_info.c
	cp -f $^ $@

es2_info: es2_info.o $(ES2_LIB_DEPS)
	$(CC) $(CFLAGS) es2_info.o $(ES2_LIBS) -o $@

tri: tri.o $(ES2_LIB_DEPS)
	$(CC) $(CFLAGS) tri.o $(ES2_LIBS) -o $@

es2gears: es2gears.o $(ES2_LIB_DEPS)
	$(CC) $(CFLAGS) es2gears.o ../eglut/libeglut-x11.a $(ES2_LIBS) -lm -o $@

clean:
	rm -f *.o *~
	rm -f $(PROGRAMS)
	rm -f es2_info.c

