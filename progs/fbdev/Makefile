# Makefile for miniglx demo programs

TOP = ../..

include $(TOP)/configs/current


SOURCES = glfbdevtest.c

OBJECTS = $(SOURCES:.c=.o)

PROGS = $(SOURCES:%.c=%)

INCLUDES = \
	-I. \
	-I$(TOP)/include


##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c:
	$(CC) $(INCLUDES) $(CFLAGS) $< $(APP_LIB_DEPS) -o $@

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $< -o $@


##### TARGETS #####

default:  depend $(PROGS)

clean:
	rm -f $(PROGS)
	rm -f *.o


depend: $(SOURCES)
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(SOURCES) > /dev/null 


# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`


include depend
