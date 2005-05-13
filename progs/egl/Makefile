# progs/egl/Makefile

TOP = ../..
include $(TOP)/configs/current


INCLUDE_DIRS = -I$(TOP)/include

HEADERS = $(TOP)/include/GLES/egl.h

PROGRAMS = \
	demo1 \
	demo2 \
	eglinfo


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: $(PROGRAMS)


demo1: demo1.o $(LIB_DIR)/libEGL.so
	$(CC) demo1.o -L$(LIB_DIR) -lEGL -o $@

demo1.o: demo1.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include demo1.c


demo2: demo2.o $(LIB_DIR)/libEGL.so
	$(CC) demo2.o -L$(LIB_DIR) -lEGL $(APP_LIB_DEPS) -o $@

demo2.o: demo2.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include demo2.c


eglinfo: eglinfo.o $(LIB_DIR)/libEGL.so
	$(CC) eglinfo.o -L$(LIB_DIR) -lEGL -o $@

eglinfo.o: eglinfo.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include eglinfo.c



clean:
	rm -f *.o *~
	rm -f *.so
	rm -f $(PROGRAMS)
