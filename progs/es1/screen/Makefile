# progs/es1/screen/Makefile

TOP = ../../..
include $(TOP)/configs/current

ES1_CFLAGS = -I$(TOP)/include
ES1_LIBS = -L$(TOP)/$(LIB_DIR) -lEGL -lGLESv1_CM

ES1_LIB_DEPS = \
	$(TOP)/$(LIB_DIR)/libEGL.so \
	$(TOP)/$(LIB_DIR)/libGLESv1_CM.so

WINSYS_OBJS = winsys.o

PROGRAMS = \
	gears \
	tri

.c.o:
	$(CC) -c $(ES1_CFLAGS) $(CFLAGS) $< -o $@

default: $(PROGRAMS)

gears: gears.o $(WINSYS_OBJS) $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) -o $@ $@.o $(WINSYS_OBJS) $(ES1_LIBS)

tri: tri.o $(WINSYS_OBJS) $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) -o $@ $@.o $(WINSYS_OBJS) $(ES1_LIBS)

clean:
	-rm -f *.o *~
	-rm -f $(PROGRAMS)
