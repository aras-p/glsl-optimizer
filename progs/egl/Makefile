# progs/egl/Makefile

TOP = ../..
include $(TOP)/configs/current


INCLUDE_DIRS = -I$(TOP)/include

HEADERS = $(TOP)/include/GLES/egl.h
LIB_DEP = $(TOP)/$(LIB_DIR)/libEGL.so

LIBS = -L$(TOP)/$(LIB_DIR) -lEGL -lGL

PROGRAMS = \
	demo1 \
	demo2 \
	demo3 \
	egltri \
	eglinfo \
	eglgears \
	eglscreen \
	peglgears \
	xeglgears \
	xeglthreads \
	xegl_tri


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: $(PROGRAMS)

demo1: demo1.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) $(LIBDRM_LIB)

demo2: demo2.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) $(LIBDRM_LIB)

demo3: demo3.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) $(LIBDRM_LIB)

egltri: egltri.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) $(LIBDRM_LIB)

eglinfo: eglinfo.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

eglgears: eglgears.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) $(LIBDRM_LIB) -lm

eglscreen: eglscreen.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) $(LIBDRM_LIB)

peglgears: peglgears.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) $(LIBDRM_LIB) -lm

xeglgears: xeglgears.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) -lm -L$(libdir) -lX11

xeglthreads: xeglthreads.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) -lm -L$(libdir) -lX11

xegl_tri: xegl_tri.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) -lm -L$(libdir) -lX11

clean:
	-rm -f *.o *~
	-rm -f $(PROGRAMS)
