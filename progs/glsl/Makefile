# progs/demos/Makefile

TOP = ../..
include $(TOP)/configs/current

INCDIR = $(TOP)/include

LIB_DEP = $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLU_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME)

LIBS = -L$(TOP)/$(LIB_DIR) -l$(GLUT_LIB) -l$(GLU_LIB) -l$(GL_LIB) $(APP_LIB_DEPS)

PROGS = \
	bitmap \
	brick \
	bump \
	convolutions \
	deriv \
	fragcoord \
	mandelbrot \
	multitex \
	noise \
	points \
	pointcoord \
	skinning \
	texdemo1 \
	toyball \
	twoside \
	trirast \
	vert-tex


##### RULES #####

.SUFFIXES:
.SUFFIXES: .c


# make executable from .c file:
.c: $(LIB_DEP)
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) $< $(LIBS) -o $@


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


shaderutil.c: $(TOP)/progs/util/shaderutil.c
	cp $< .

shaderutil.h: $(TOP)/progs/util/shaderutil.h
	cp $< .

shaderutil.o: shaderutil.c shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) shaderutil.c



bitmap.o: bitmap.c extfuncs.h shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) bitmap.c

bitmap: bitmap.o shaderutil.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) bitmap.o shaderutil.o $(LIBS) -o $@


brick.o: brick.c extfuncs.h shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) brick.c

brick: brick.o shaderutil.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) brick.o shaderutil.o $(LIBS) -o $@


bump.o: bump.c extfuncs.h shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) bump.c

bump: bump.o shaderutil.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) bump.o shaderutil.o $(LIBS) -o $@


convolutions.o: convolutions.c readtex.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) convolutions.c

convolutions: convolutions.o readtex.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) convolutions.o readtex.o $(LIBS) -o $@


deriv.o: deriv.c extfuncs.h shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) deriv.c

deriv: deriv.o shaderutil.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) deriv.o shaderutil.o $(LIBS) -o $@


fragcoord.o: fragcoord.c extfuncs.h shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) fragcoord.c

fragcoord: fragcoord.o shaderutil.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) fragcoord.o shaderutil.o $(LIBS) -o $@


mandelbrot.o: mandelbrot.c extfuncs.h shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) mandelbrot.c

mandelbrot: mandelbrot.o shaderutil.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) mandelbrot.o shaderutil.o $(LIBS) -o $@


multitex.o: multitex.c extfuncs.h readtex.h shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) multitex.c

multitex: multitex.o readtex.o shaderutil.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) multitex.o readtex.o shaderutil.o $(LIBS) -o $@


noise.o: noise.c extfuncs.h shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) noise.c

noise: noise.o shaderutil.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) noise.o shaderutil.o $(LIBS) -o $@


points.o: points.c extfuncs.h shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) points.c

points: points.o shaderutil.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) points.o shaderutil.o $(LIBS) -o $@


pointcoord.o: pointcoord.c readtex.h extfuncs.h shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) pointcoord.c

pointcoord: pointcoord.o readtex.o shaderutil.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) pointcoord.o readtex.o shaderutil.o $(LIBS) -o $@


skinning.o: skinning.c readtex.h extfuncs.h shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) skinning.c

skinning: skinning.o readtex.o shaderutil.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) skinning.o readtex.o shaderutil.o $(LIBS) -o $@


texdemo1.o: texdemo1.c readtex.h extfuncs.h shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) texdemo1.c

texdemo1: texdemo1.o readtex.o shaderutil.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) texdemo1.o readtex.o shaderutil.o $(LIBS) -o $@


toyball.o: toyball.c extfuncs.h shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) toyball.c

toyball: toyball.o shaderutil.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) toyball.o shaderutil.o $(LIBS) -o $@


twoside.o: twoside.c extfuncs.h shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) twoside.c

twoside: twoside.o shaderutil.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) twoside.o shaderutil.o $(LIBS) -o $@


trirast.o: trirast.c extfuncs.h shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) trirast.c

trirast: trirast.o shaderutil.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) trirast.o shaderutil.o $(LIBS) -o $@


vert-tex.o: vert-tex.c extfuncs.h shaderutil.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) vert-tex.c

vert-tex: vert-tex.o shaderutil.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) vert-tex.o shaderutil.o $(LIBS) -o $@




clean:
	-rm -f $(PROGS)
	-rm -f *.o *~
	-rm -f extfuncs.h
	-rm -f shaderutil.*
