# src/mapi/glapi/Makefile

TOP = ../../..
include $(TOP)/configs/current

include sources.mak
GLAPI_OBJECTS = $(GLAPI_SOURCES:.c=.o)
GLAPI_ASM_OBJECTS = $(GLAPI_ASM_SOURCES:.S=.o)

INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mapi \
	-I$(TOP)/src/mesa

default: depend libglapi.a

libglapi.a: $(GLAPI_OBJECTS) $(GLAPI_ASM_OBJECTS)
	@ $(MKLIB) -o glapi -static $(GLAPI_OBJECTS) $(GLAPI_ASM_OBJECTS)

$(GLAPI_OBJECTS): %.o: %.c
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@

$(GLAPI_ASM_OBJECTS): %.o: %.S
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@

install:

clean:
	-rm -f $(GLAPI_OBJECTS) $(GLAPI_ASM_OBJECTS)
	-rm -f depend depend.bak libglapi.a

depend: $(GLAPI_SOURCES)
	@ echo "running $(MKDEP)"
	@ touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) $(GLAPI_SOURCES) \
		> /dev/null 2>/dev/null

-include depend
