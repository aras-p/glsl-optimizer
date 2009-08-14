# progs/demos/Makefile

TOP = ../..
include $(TOP)/configs/current

INCDIR = $(TOP)/include

LIB_DEP = \
	$(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) \
	$(TOP)/$(LIB_DIR)/$(GLU_LIB_NAME) \
	$(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME)

LIBS = -L$(TOP)/$(LIB_DIR) -l$(GLUT_LIB) -l$(GLEW_LIB) -l$(GLU_LIB) -l$(GL_LIB) $(APP_LIB_DEPS)

INCLUDE_DIRS = -I$(TOP)/progs/util

# using : to avoid APP_CC pointing to CC loop
CC:=$(APP_CC)
CFLAGS += -I$(INCDIR)
LDLIBS=$(LIBS)

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
	shtest.c \
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
PROG_OBJS = $(DEMO_SOURCES:.c=.o)
PROGS = $(DEMO_SOURCES:%.c=%)

##### TARGETS #####

default: $(PROGS)

$(PROG_OBJS): $(UTIL_HEADERS)

$(PROGS): $(UTIL_OBJS)

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

