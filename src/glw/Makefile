# src/glw/Makefile

TOP = ../..
include $(TOP)/configs/current

MAJOR = 1
MINOR = 0
TINY = 0

INCDIRS = -I$(TOP)/include -I/usr/include/Motif1.2 $(X11_INCLUDES)


OBJECTS = $(GLW_SOURCES:.c=.o)



##### RULES #####

.c.o:
	$(CC) -c $(INCDIRS) $(CFLAGS) $<



##### TARGETS #####

default: $(LIB_DIR)/$(GLW_LIB_NAME)


clean:
	-rm depend depend.bak
	-rm -f *.o *~


# Make the library
$(LIB_DIR)/$(GLW_LIB_NAME): $(OBJECTS)
	$(TOP)/bin/mklib -o $(GLW_LIB) -linker '$(CC)' \
		-major $(MAJOR) -minor $(MINOR) -patch $(TINY) \
		$(MKLIB_OPTIONS) -install $(LIB_DIR) \
		$(GLW_LIB_DEPS) $(OBJECTS)


#
# Run 'make depend' to update the dependencies if you change what's included
# by any source file.
# 
depend: $(GLW_SOURCES)
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) -I$(TOP)/include $(GLW_SOURCES) \
		> /dev/null 


include depend
