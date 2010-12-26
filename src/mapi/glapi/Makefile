# src/mapi/glapi/Makefile

TOP = ../../..
include $(TOP)/configs/current

TARGET = glapi

MAPI = $(TOP)/src/mapi/mapi

include sources.mak
include $(MAPI)/sources.mak

glapi_CPPFLAGS := \
	-I$(TOP)/include \
	-I$(TOP)/src/mapi \
	-I$(TOP)/src/mesa \
	-DMAPI_ABI_HEADER=\"glapi/glapi_mapi_tmp.h\"

ifeq ($(SHARED_GLAPI),1)
glapi_CPPFLAGS += -DMAPI_MODE_BRIDGE
glapi_SOURCES := $(addprefix $(MAPI)/, $(MAPI_BRIDGE_SOURCES))

glapi_GLAPI_OBJECTS :=
glapi_ASM_OBJECTS :=
glapi_MAPI_OBJECTS := $(MAPI_BRIDGE_SOURCES:.c=.o)
else
glapi_CPPFLAGS += -DMAPI_MODE_UTIL
glapi_SOURCES := $(GLAPI_SOURCES) $(addprefix $(MAPI)/, $(MAPI_UTIL_SOURCES))

glapi_GLAPI_OBJECTS := $(GLAPI_SOURCES:.c=.o)
glapi_ASM_OBJECTS := $(GLAPI_ASM_SOURCES:.S=.o)
glapi_MAPI_OBJECTS := $(MAPI_UTIL_SOURCES:.c=.o)
endif # SHARED_GLAPI

glapi_OBJECTS := \
	$(glapi_GLAPI_OBJECTS) \
	$(glapi_ASM_OBJECTS) \
	$(glapi_MAPI_OBJECTS)

default: depend lib$(TARGET).a

lib$(TARGET).a: $(glapi_OBJECTS)
	@$(MKLIB) -o $(TARGET) -static $(glapi_OBJECTS)

$(glapi_GLAPI_OBJECTS): %.o: %.c
	$(CC) -c $(glapi_CPPFLAGS) $(CFLAGS) $< -o $@

$(glapi_ASM_OBJECTS): %.o: %.S
	$(CC) -c $(glapi_CPPFLAGS) $(CFLAGS) $< -o $@

$(glapi_MAPI_OBJECTS): %.o: $(MAPI)/%.c
	$(CC) -c $(glapi_CPPFLAGS) $(CFLAGS) $< -o $@

install:

clean:
	-rm -f $(glapi_OBJECTS)
	-rm -f lib$(TARGET).a
	-rm -f depend depend.bak

depend: $(glapi_SOURCES)
	@ echo "running $(MKDEP)"
	@ touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) -f- $(DEFINES) $(glapi_CPPFLAGS) \
		$(glapi_SOURCES) 2>/dev/null | sed -e 's,^$(MAPI)/,,' > depend

-include depend
