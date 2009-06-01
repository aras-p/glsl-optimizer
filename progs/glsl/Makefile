# progs/demos/Makefile

TOP = ../..
include $(TOP)/configs/current

INCDIR = $(TOP)/include

LIB_DEP = \
	$(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) \
	$(TOP)/$(LIB_DIR)/$(GLU_LIB_NAME) \
	$(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME)

LIBS = -L$(TOP)/$(LIB_DIR) -l$(GLUT_LIB) -l$(GLU_LIB) -l$(GL_LIB) $(APP_LIB_DEPS)

INCLUDE_DIRS = -I$(TOP)/progs/util

DEMO_SOURCES = \
	array.c \
	bitmap.c \
	brick.c \
	bump.c \
	convolutions.c \
	deriv.c \
	fragcoord.c \
	identity.c \
	linktest.c \
	mandelbrot.c \
	multinoise.c \
	multitex.c \
	noise.c \
	noise2.c \
	pointcoord.c \
	points.c \
	samplers.c \
	shadow_sampler.c \
	skinning.c \
	texaaline.c \
	texdemo1.c \
	toyball.c \
	trirast.c \
	twoside.c \
	vert-or-frag-only.c \
	vert-tex.c

UTIL_HEADERS = \
	extfuncs.h \
	shaderutil.h \
	readtex.h

UTIL_SOURCES = \
	shaderutil.c \
	readtex.c

UTIL_OBJS = $(UTIL_SOURCES:.c=.o)


PROGS = $(DEMO_SOURCES:%.c=%)



##### RULES #####

# make .o file from .c file:
.c.o:
	$(APP_CC) -c -I$(INCDIR) $(CFLAGS) $< -o $@


# make executable from .o files
.o:
	$(APP_CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $< $(UTIL_OBJS) $(LIBS) -o $@


##### TARGETS #####

default: $(PROGS)


clean:
	-rm -f $(PROGS)
	-rm -f *.o *~
	-rm -f extfuncs.h
	-rm -f shaderutil.*
	-rm -f readtex.*



##### Extra dependencies

extfuncs.h: $(TOP)/progs/util/extfuncs.h
	cp $< .

readtex.c: $(TOP)/progs/util/readtex.c
	cp $< .

readtex.h: $(TOP)/progs/util/readtex.h
	cp $< .

shaderutil.c: $(TOP)/progs/util/shaderutil.c
	cp $< .

shaderutil.h: $(TOP)/progs/util/shaderutil.h
	cp $< .



array.o: $(UTIL_HEADERS)

array: array.o  $(UTIL_OBJS)


bitmap.o: $(UTIL_HEADERS)

bitmap: bitmap.o $(UTIL_OBJS)


brick.o: $(UTIL_HEADERS)

brick: brick.o $(UTIL_OBJS)


bump.o: $(UTIL_HEADERS)

bump: bump.o $(UTIL_OBJS)


convolutions.o: $(UTIL_HEADERS)

convolutions: convolutions.o $(UTIL_OBJS)


deriv.o: deriv.c $(UTIL_HEADERS)

deriv: deriv.o $(UTIL_OBJS)


identity.o: $(UTIL_HEADERS)

identity: identity.o $(UTIL_OBJS)


fragcoord.o: $(UTIL_HEADERS)

fragcoord: fragcoord.o $(UTIL_OBJS)


linktest.o: $(UTIL_HEADERS)

linktest: linktest.o $(UTIL_OBJS)


mandelbrot.o: $(UTIL_HEADERS)

mandelbrot: mandelbrot.o $(UTIL_OBJS)


multinoise.o: $(UTIL_HEADERS)

multinoise: multinoise.o $(UTIL_OBJS)


multitex.o: $(UTIL_HEADERS)

multitex: multitex.o $(UTIL_OBJS)


noise.o: $(UTIL_HEADERS)

noise: noise.o $(UTIL_OBJS)


noise2.o: $(UTIL_HEADERS)

noise2: noise2.o $(UTIL_OBJS)


points.o: $(UTIL_HEADERS)

points: points.o $(UTIL_OBJS)


pointcoord.o: $(UTIL_HEADERS)

pointcoord: pointcoord.o $(UTIL_OBJS)


samplers.o: $(UTIL_HEADERS)

samplers: samplers.o $(UTIL_OBJS)


samplers_array.o: $(UTIL_HEADERS)

samplers_array: samplers_array.o $(UTIL_OBJS)


shadow_sampler.o: $(UTIL_HEADERS)

shadow_sampler: shadow_sampler.o $(UTIL_OBJS)


skinning.o: $(UTIL_HEADERS)

skinning: skinning.o $(UTIL_OBJS)


texaaline.o: $(UTIL_HEADERS)

texaaline: texaaline.o $(UTIL_OBJS)


texdemo1.o: $(UTIL_HEADERS)

texdemo1: texdemo1.o $(UTIL_OBJS)


toyball.o: $(UTIL_HEADERS)

toyball: toyball.o $(UTIL_OBJS)


twoside.o: $(UTIL_HEADERS)

twoside: twoside.o $(UTIL_OBJS)


trirast.o: $(UTIL_HEADERS)

trirast: trirast.o $(UTIL_OBJS)


vert-or-frag-only.o: $(UTIL_HEADERS)

vert-or-frag-only: vert-or-frag-only.o $(UTIL_OBJS)


vert-tex.o: $(UTIL_HEADERS)

vert-tex: vert-tex.o $(UTIL_OBJS)
