# src/mapi/glapi/Makefile

TOP = ../../..
include $(TOP)/configs/current

TARGET = glapi

MAPI = $(TOP)/src/mapi/mapi

include sources.mak
GLAPI_OBJECTS = $(GLAPI_SOURCES:.c=.o)
GLAPI_ASM_OBJECTS = $(GLAPI_ASM_SOURCES:.S=.o)

include $(MAPI)/sources.mak
MAPI_UTIL_OBJECTS := $(MAPI_UTIL_SOURCES:.c=.o)
MAPI_UTIL_SOURCES := $(addprefix $(MAPI)/, $(MAPI_UTIL_SOURCES))

TARGET_OBJECTS = $(GLAPI_OBJECTS) $(GLAPI_ASM_OBJECTS) $(MAPI_UTIL_OBJECTS)

INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mapi \
	-I$(TOP)/src/mesa

default: depend lib$(TARGET).a

lib$(TARGET).a: $(TARGET_OBJECTS)
	@$(MKLIB) -o $(TARGET) -static $(TARGET_OBJECTS)

$(GLAPI_OBJECTS): %.o: %.c
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) -DMAPI_MODE_UTIL $< -o $@

$(GLAPI_ASM_OBJECTS): %.o: %.S
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@

$(MAPI_UTIL_OBJECTS): %.o: $(MAPI)/%.c
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) -DMAPI_MODE_UTIL $< -o $@

install:

clean:
	-rm -f $(TARGET_OBJECTS)
	-rm -f lib$(TARGET).a
	-rm -f depend depend.bak

depend: $(GLAPI_SOURCES) $(MAPI_UTIL_SOURCES)
	@ echo "running $(MKDEP)"
	@ touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) -f- $(DEFINES) $(INCLUDE_DIRS) \
		-DMAPI_MODE_UTIL $(GLAPI_SOURCES) $(MAPI_UTIL_SOURCES) \
		2>/dev/null | sed -e 's,^$(MAPI)/,,' > depend

-include depend
