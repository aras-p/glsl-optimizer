# progs/tests/Makefile


# These programs aren't intended to be included with the normal distro.
# They're not too interesting but they're good for testing.

TOP = ../..
include $(TOP)/configs/current

LIBS = -L$(TOP)/$(LIB_DIR) -l$(GLUT_LIB)  -l$(GLEW_LIB) -l$(GLU_LIB) -l$(GL_LIB) $(APP_LIB_DEPS)

SOURCES = \
	fp-tri.c



PROGS = $(SOURCES:%.c=%)

INCLUDES = -I. -I$(TOP)/include -I../samples


##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c:
	$(CC) $(INCLUDES) $(CFLAGS) $< $(LIBS) -o $@

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES)  $< -o $@


##### TARGETS #####

default: $(PROGS)

clean:
	rm -f $(PROGS)
	rm -f *.o
	rm -f getproclist.h





# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`
