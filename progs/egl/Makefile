# progs/egl/Makefile

TOP = ../..
include $(TOP)/configs/current


INCLUDE_DIRS = -I$(TOP)/include

HEADERS = $(TOP)/include/GLES/egl.h

PROGRAMS = \
	demo1 \
	demo2 \
	demo3 \
	egltri \
	eglinfo \
	eglgears \
	eglscreen \
	peglgears \
	xeglbindtex \
	xeglgears \
	xeglthreads \
	xegl_tri


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: $(PROGRAMS)


demo1: demo1.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) $(LDFLAGS) demo1.o -L$(TOP)/$(LIB_DIR) -lEGL $(LIBDRM_LIB) -o $@

demo1.o: demo1.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include demo1.c


demo2: demo2.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) $(LDFLAGS) demo2.o -L$(TOP)/$(LIB_DIR) -lEGL -lGL $(LIBDRM_LIB) $(APP_LIB_DEPS) -o $@

demo2.o: demo2.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include demo2.c


demo3: demo3.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) $(LDFLAGS) demo3.o -L$(TOP)/$(LIB_DIR) -lEGL -lGL $(LIBDRM_LIB) $(APP_LIB_DEPS) -o $@

demo3.o: demo3.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include demo3.c


egltri: egltri.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) egltri.o -L$(TOP)/$(LIB_DIR) -lEGL -lGL $(LIBDRM_LIB) -o $@

egltri.o: egltri.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include egltri.c


eglinfo: eglinfo.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) $(LDFLAGS) eglinfo.o -L$(TOP)/$(LIB_DIR) -lEGL -lGL $(LIBDRM_LIB) -o $@

eglinfo.o: eglinfo.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include eglinfo.c


eglgears: eglgears.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) $(LDFLAGS) eglgears.o -L$(TOP)/$(LIB_DIR) -lEGL -lGL $(LIBDRM_LIB) $(APP_LIB_DEPS) -o $@

eglgears.o: eglgears.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include eglgears.c

eglscreen: eglscreen.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) $(LDFLAGS) eglscreen.o -L$(TOP)/$(LIB_DIR) -lEGL -lGL $(LIBDRM_LIB) $(APP_LIB_DEPS) -o $@

eglscreen.o: eglscreen.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include eglscreen.c

peglgears: peglgears.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) peglgears.o -L$(TOP)/$(LIB_DIR) -lEGL -lGL $(LIBDRM_LIB) $(APP_LIB_DEPS) -o $@

peglgears.o: peglgears.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include peglgears.c


xeglbindtex: xeglbindtex.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) xeglbindtex.o -L$(TOP)/$(LIB_DIR) -lEGL -lGL $(LIBDRM_LIB) $(APP_LIB_DEPS) -o $@

xeglbindtex.o: xeglbindtex.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include xeglbindtex.c

xeglgears: xeglgears.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) xeglgears.o -L$(TOP)/$(LIB_DIR) -lEGL -lGL $(LIBDRM_LIB) $(APP_LIB_DEPS) -o $@

xeglgears.o: xeglgears.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include xeglgears.c


xeglthreads: xeglthreads.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) xeglthreads.o -L$(TOP)/$(LIB_DIR) -lEGL -lGL $(LIBDRM_LIB) $(APP_LIB_DEPS) -o $@

xeglthreads.o: xeglthreads.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include xeglthreads.c


xegl_tri: xegl_tri.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) xegl_tri.o -L$(TOP)/$(LIB_DIR) -lEGL -lGL $(LIBDRM_LIB) $(APP_LIB_DEPS) -o $@

xegl_tri.o: xegl_tri.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include xegl_tri.c


clean:
	-rm -f *.o *~
	-rm -f *.so
	-rm -f $(PROGRAMS)

