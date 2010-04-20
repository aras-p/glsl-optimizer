# progs/egl/eglut

TOP = ../../..
include $(TOP)/configs/current

INCLUDES = \
	-I$(TOP)/include \
	$(X11_CFLAGS)

SOURCES = $(wildcard *.c)

EGLUT_X11_OBJECTS = eglut.o eglut_x11.o
EGLUT_SCREEN_OBJECTS = eglut.o eglut_screen.o

default: depend libeglut-x11.a libeglut-screen.a

libeglut-x11.a: $(EGLUT_X11_OBJECTS)
	$(MKLIB) -o eglut-x11 -static $(EGLUT_X11_OBJECTS)

libeglut-screen.a: $(EGLUT_SCREEN_OBJECTS)
	$(MKLIB) -o eglut-screen -static $(EGLUT_SCREEN_OBJECTS)

.c.o:
	$(CC) -c -o $@ $< $(INCLUDES) $(DEFINES) $(CFLAGS)

depend: $(SOURCES)
	@rm -f depend
	@touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(SOURCES) \
		> /dev/null 2>/dev/null

clean:
	rm -f *.o *.a
	rm -f depend depend.bak

sinclude depend
