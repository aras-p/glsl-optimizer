# progs/demos/Makefile

TOP = ../..
include $(TOP)/configs/current

INCDIR = $(TOP)/include

OSMESA_LIBS = -L$(TOP)/$(LIB_DIR) -lglut -lOSMesa -lGLU -lGL $(APP_LIB_DEPS)

OSMESA16_LIBS = -L$(TOP)/$(LIB_DIR) -lglut -lOSMesa16 -lGLU -lGL $(APP_LIB_DEPS)

OSMESA32_LIBS = -L$(TOP)/$(LIB_DIR) -lglut -lOSMesa32 -lGLU -lGL $(APP_LIB_DEPS)

LIB_DEP = $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLU_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME)

PROGS = \
	brick \
	bump \
	mandelbrot \
	noise \
	toyball


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


brick.c: extfuncs.h

bump.c: extfuncs.h

mandelbrot.c: extfuncs.h

toyball.c: extfuncs.h


clean:
	-rm -f $(PROGS)
	-rm -f *.o *~
	-rm -f extfuncs.h
