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
	peglgears \
	xeglgears \
	xegl_tri


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: $(PROGRAMS)


demo1: demo1.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) demo1.o -L$(TOP)/$(LIB_DIR) -lEGL $(LIBDRM_LIB) -o $@

demo1.o: demo1.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include demo1.c


demo2: demo2.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) demo2.o -L$(TOP)/$(LIB_DIR) -lEGL $(LIBDRM_LIB) $(APP_LIB_DEPS) -o $@

demo2.o: demo2.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include demo2.c


demo3: demo3.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) demo3.o -L$(TOP)/$(LIB_DIR) -lEGL $(LIBDRM_LIB) $(APP_LIB_DEPS) -o $@

demo3.o: demo3.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include demo3.c


egltri: egltri.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) egltri.o -L$(TOP)/$(LIB_DIR) -lGL -lEGL $(LIBDRM_LIB) -o $@

egltri.o: egltri.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include egltri.c


eglinfo: eglinfo.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) eglinfo.o -L$(TOP)/$(LIB_DIR) -lGL -lEGL $(LIBDRM_LIB) -o $@

eglinfo.o: eglinfo.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include eglinfo.c


eglgears: eglgears.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) eglgears.o -L$(TOP)/$(LIB_DIR) -lEGL $(LIBDRM_LIB) $(APP_LIB_DEPS) -o $@

eglgears.o: eglgears.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include eglgears.c


peglgears: peglgears.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) peglgears.o -L$(TOP)/$(LIB_DIR) -lEGL $(LIBDRM_LIB) $(APP_LIB_DEPS) -o $@

peglgears.o: peglgears.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include peglgears.c


xeglgears: xeglgears.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) xeglgears.o -L$(TOP)/$(LIB_DIR) -lEGL $(LIBDRM_LIB) $(APP_LIB_DEPS) -o $@

xeglgears.o: xeglgears.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include xeglgears.c


xegl_tri: xegl_tri.o $(TOP)/$(LIB_DIR)/libEGL.so
	$(CC) $(CFLAGS) xegl_tri.o -L$(TOP)/$(LIB_DIR) -lEGL $(LIBDRM_LIB) $(APP_LIB_DEPS) -o $@

xegl_tri.o: xegl_tri.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include xegl_tri.c


clean:
	rm -f *.o *~
	rm -f *.so
	rm -f $(PROGRAMS)

run:
	LD_LIBRARY_PATH=$(TOP)/lib ./egltri

debug:
	LD_LIBRARY_PATH=$(TOP)/lib gdb ./eglgears
