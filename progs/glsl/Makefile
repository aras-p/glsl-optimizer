# progs/demos/Makefile

TOP = ../..
include $(TOP)/configs/current

INCDIR = $(TOP)/include

LIB_DEP = $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLU_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME)

PROGS = \
	brick \
	bump \
	deriv \
	mandelbrot \
	noise \
	toyball \
	texdemo1


##### RULES #####

.SUFFIXES:
.SUFFIXES: .c


# make executable from .c file:
.c: $(LIB_DEP)
	$(CC) -I$(INCDIR) $(CFLAGS) $< $(APP_LIB_DEPS) -o $@


##### TARGETS #####

default: $(PROGS)



##### Extra dependencies

extfuncs.h:  $(TOP)/progs/util/extfuncs.h
	cp $< .

readtex.c: $(TOP)/progs/util/readtex.c
	cp $< .

readtex.h: $(TOP)/progs/util/readtex.h
	cp $< .

readtex.o: readtex.c readtex.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) readtex.c

brick.c: extfuncs.h

bump.c: extfuncs.h

mandelbrot.c: extfuncs.h

toyball.c: extfuncs.h

texdemo1: texdemo1.o readtex.o
	$(CC) -I$(INCDIR) $(CFLAGS) texdemo1.o readtex.o $(APP_LIB_DEPS) -o $@

texdemo1.o: texdemo1.c readtex.h extfuncs.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) texdemo1.c


clean:
	-rm -f $(PROGS)
	-rm -f *.o *~
	-rm -f extfuncs.h
