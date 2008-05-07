# Makefile for miniglx demo programs

TOP = ../..

include $(TOP)/configs/current


SOURCES = glfbdevtest.c

OBJECTS = $(SOURCES:.c=.o)

PROGS = $(SOURCES:%.c=%)

INCLUDES = \
	-I. \
	-I$(TOP)/include

LIBS = -L$(TOP)/$(LIB_DIR) -l$(GLU_LIB) -l$(GL_LIB) $(APP_LIB_DEPS)

##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c:
	$(CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $< $(LIBS) -o $@

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $< -o $@


##### TARGETS #####

default:  depend $(PROGS)

clean:
	-rm -f $(PROGS)
	-rm -f *.o


depend: $(SOURCES)
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(SOURCES) > /dev/null 


# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`


include depend
