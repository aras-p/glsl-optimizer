# Makefile for OpenGL widgets

# NOTE: widget code is from SGI.  See any of the .c or .h files for the
# complete copyright.  Mesa's GNU copyright DOES NOT apply to this widget
# code.


##### MACROS #####

VPATH = RCS

INCDIRS = -I../include -I/usr/include/Motif1.2 -I/usr/X11R6/include
LIBDIR = ../lib

SOURCES = GLwDrawA.c GLwMDrawA.c


OBJECTS = $(SOURCES:.c=.o)



##### RULES #####

.c.o:
	$(CC) -c $(INCDIRS) $(CFLAGS) $<



##### TARGETS #####

default:
	@echo "Specify a target configuration"

clean:
	-rm *.o *~

# The name of the library file comes from Make-config
#XXX GLW_LIB = libGLw.a

targets: $(LIBDIR)/$(GLW_LIB)


# Make the library
$(LIBDIR)/$(GLW_LIB): $(OBJECTS)
	$(MAKELIB) $(GLW_LIB) $(MAJOR) $(MINOR) $(OBJECTS)
	mv $(GLW_LIB)* $(LIBDIR)

include ../Make-config

include depend



#
# Run 'make depend' to update the dependencies if you change what's included
# by any source file.
# 
dep: $(SOURCES)
	makedepend -fdepend -Y -I../include $(SOURCES)
