# progs/vg/Makefile

TOP = ../../..
include $(TOP)/configs/current

VG_LIBS=-lm -pthread -lEGL -lOpenVG
INCLUDE_DIRS = -I$(TOP)/include

PROGRAMS = \
        lion \
        sp

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: $(PROGRAMS)

lion: lion.o lion-render.o
	$(CC) $(CFLAGS) lion.o lion-render.o -L$(TOP)/$(LIB_DIR) $(VG_LIBS) -o $@

lion.o: lion.c lion-render.h $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include lion.c
lion-render.o: lion-render.c lion-render.h $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include lion-render.c


sp: sp.c eglcommon.o
	$(CC) $(INCLUDE_DIRS) $(CFLAGS) $^  -L$(TOP)/$(LIB_DIR) $(LIBS) $(VG_LIBS)  $(APP_LIB_DEPS) -o $@

eglcommon.o: eglcommon.c $(HEADERS)
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) eglcommon.c


clean:
	rm -f *.o *~
	rm -f *.so
	rm -f $(PROGRAMS)
