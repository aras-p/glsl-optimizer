# Makefile for GLUT for ggi

TOP = ../../..

include $(TOP)/configs/current


##### MACROS #####

GLUT_MAJOR = 3
GLUT_MINOR = 7
GLUT_TINY = 1

SOURCES = \
	ggiglut.c


OBJECTS = $(SOURCES:.c=.o)


##### RULES #####

.c.o:
	$(CC) -c -I$(TOP)/include $(CFLAGS) $(GLUT_CFLAGS) $<



##### TARGETS #####

default: $(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME)


# Make the library
$(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME): depend $(OBJECTS)
	$(MKLIB) -o $(GLUT_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(GLUT_MAJOR) -minor $(GLUT_MINOR) \
		-patch $(GLUT_TINY) $(MKLIB_OPTIONS) -install $(TOP)/$(LIB_DIR) \
		$(GLUT_LIB_DEPS) $(OBJECTS)


clean:
	-rm -f *.o *~
	-rm -f *.lo
	-rm -f *.la
	-rm -rf .libs


depend: $(SOURCES)
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) -I$(TOP)/include $(SOURCES)

include depend
