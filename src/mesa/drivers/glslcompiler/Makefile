# Makefile for stand-alone GL-SL compiler

TOP = ../../../..

include $(TOP)/configs/current


PROGRAM = glslcompiler

OBJECTS = \
	glslcompiler.o \
	../common/driverfuncs.o \
	../../libmesa.a \
	../../libglapi.a

INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/include/GL/internal \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa/math \
	-I$(TOP)/src/mesa/transform \
	-I$(TOP)/src/mesa/shader \
	-I$(TOP)/src/mesa/swrast \
	-I$(TOP)/src/mesa/swrast_setup \


default: $(PROGRAM)
	$(INSTALL) $(PROGRAM) $(TOP)/bin


glslcompiler: $(OBJECTS)
	$(CC) $(OBJECTS) $(GL_LIB_DEPS) -o $@


glslcompiler.o: glslcompiler.c
	$(CC) -c $(INCLUDES) $(CFLAGS) glslcompiler.c -o $@


clean:
	-rm -f *.o *~ $(PROGRAM)
