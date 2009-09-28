TOP = ../..
include $(TOP)/configs/current

#DEFINES += -DDEFAULT_DRIVER_DIR=\"$(DRI_DRIVER_SEARCH_DIR)\"

SOURCES = block.c \
          surface.c \
          context.c \
          subpicture.c \
          attributes.c

OBJECTS = $(SOURCES:.c=.o)

INCLUDES = -I$(TOP)/src/gallium/include \
           -I$(TOP)/src/gallium/auxiliary \
           -I$(TOP)/src/gallium/winsys/g3dvl

##### RULES #####

.c.o:
	$(CC) -c $(INCLUDES) $(DEFINES) $(CFLAGS) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(DEFINES) $(CFLAGS) $< -o $@

##### TARGETS #####

.PHONY: default clean

default: depend libXvMCapi.a

libXvMCapi.a: $(OBJECTS) Makefile
	$(MKLIB) -o XvMCapi $(MKLIB_OPTIONS) -static $(OBJECTS)

depend: $(SOURCES) Makefile
	$(RM) depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDES) $(SOURCES)

clean: Makefile
	$(RM) libXvMCapi.a
	$(RM) *.o *~
	$(RM) depend depend.bak

-include depend
