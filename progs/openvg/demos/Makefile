# progs/vg/Makefile

TOP = ../../..
include $(TOP)/configs/current

VG_LIBS=-lm -pthread -lEGL -lOpenVG
INCLUDE_DIRS = -I$(TOP)/include

PROGRAMS = \
	gears \
        lion

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: $(PROGRAMS)


gears: gears.o
	$(CC) $(CFLAGS) gears.o -L$(TOP)/$(LIB_DIR) $(VG_LIBS)  -o $@

gears.o: gears.c $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include gears.c


lion: lion.o lion-render.o
	$(CC) $(CFLAGS) lion.o lion-render.o -L$(TOP)/$(LIB_DIR) $(VG_LIBS) -o $@

lion.o: lion.c lion-render.h $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include lion.c
lion-render.o: lion-render.c lion-render.h $(HEADERS)
	$(CC) -c $(CFLAGS) -I$(TOP)/include lion-render.c


clean:
	rm -f *.o *~
	rm -f *.so
	rm -f $(PROGRAMS)
