# progs/demos/Makefile

TOP = ../..
include $(TOP)/configs/current

INCDIR = $(TOP)/include

LIBS = -L$(TOP)/$(LIB_DIR) -l$(GLUT_LIB) -l$(GLEW_LIB) \
	-l$(GLU_LIB) -l$(GL_LIB) $(APP_LIB_DEPS)

# using : to avoid APP_CC pointing to CC loop
CC := $(APP_CC)
CFLAGS := -I$(INCDIR) $(CFLAGS)
LDLIBS = $(LIBS)

PROG_SOURCES = \
	copytex.c \
	drawoverhead.c \
	fbobind.c \
	fill.c \
	genmipmap.c \
	readpixels.c \
	swapbuffers.c \
	teximage.c \
	vbo.c \
	vertexrate.c \

PROG_OBJS = $(PROG_SOURCES:.c=.o)

PROGS = $(PROG_SOURCES:%.c=%)


UTIL_SOURCES = \
	common.c \
	glmain.c

UTIL_HEADERS = \
	common.h \
	glmain.h

UTIL_OBJS = $(UTIL_SOURCES:.c=.o)



default: $(PROGS)

$(PROG_OBJS): $(UTIL_HEADERS)

$(PROGS): $(UTIL_OBJS)



clean:
	-rm -f $(PROGS)
	-rm -f *.o *~
