TOP = ../..
include $(TOP)/configs/current

INCDIR = $(TOP)/include

LIBS = -L$(TOP)/$(LIB_DIR) -l$(GLUT_LIB) -l$(GLEW_LIB) \
	-l$(GLU_LIB) -l$(GL_LIB) $(APP_LIB_DEPS)

# using : to avoid APP_CC pointing to CC loop
CC := $(APP_CC)
CFLAGS := -I$(INCDIR) $(CFLAGS)
LDLIBS = $(LIBS)

SOURCES = \
	glm.c \
	glmdraw.c \
	objview.c \
	trackball.c \
	skybox.c \
	readtex.c \
	shaderutil.c

HEADERS = \
	extfuncs.h \
	readtex.h \
	shaderutil.h \
	trackball.h

OBJS = $(SOURCES:.c=.o)

PROG = objview


.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

default: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(OBJS): $(HEADERS)


### Extra dependencies

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

trackball.c: $(TOP)/progs/util/trackball.c
	cp $< .

trackball.h: $(TOP)/progs/util/trackball.h
	cp $< .


clean:
	-rm -f $(PROG) $(OBJS)
	-rm -f *.o *~
	-rm -f extfuncs.h
	-rm -f shaderutil.*
	-rm -f trackball.*
	-rm -f readtex.*


depend: $(SOURCES)
	@$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDE_DIRS) $(SOURCES) \
		> /dev/null 2>/dev/null


-include depend
