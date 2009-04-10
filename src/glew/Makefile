# src/glew/Makefile

TOP = ../..
include $(TOP)/configs/current


##### MACROS #####

GLEW_LIB_NAME = libGLEW.a

MAJOR = 1
MINOR = 5
TINY = 0

SOURCES = \
	glew.c


OBJECTS = $(SOURCES:.c=.o)


##### RULES #####

.c.o:
	$(CC) -c -I$(TOP)/include $(X11_INCLUDES) $(CFLAGS) $<



##### TARGETS #####

default: $(TOP)/$(LIB_DIR)/$(GLEW_LIB_NAME)



# Make the library
$(TOP)/$(LIB_DIR)/$(GLEW_LIB_NAME): depend $(OBJECTS)
	$(MKLIB) -o $(GLEW_LIB) -static -install $(TOP)/$(LIB_DIR) $(OBJECTS)


install:


clean:
	-rm -f *.o *~
	-rm -f depend depend.bak


depend: $(SOURCES)
	@ rm -f depend
	@ touch depend
	@ $(MKDEP) $(MKDEP_OPTIONS) -I$(TOP)/include $(SOURCES) \
		$(X11_INCLUDES) > /dev/null

include depend
