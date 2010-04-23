# src/mapi/es1api/Makefile

TOP := ../../..
include $(TOP)/configs/current

# this Makefile can build both libes1api.a and libes2api.a
ifeq ($(ES),)
ES := es1
endif

TARGET = $(ES)api

GLAPI := $(TOP)/src/mapi/glapi
MAPI := $(TOP)/src/mapi/mapi
# directory for generated sources/headers
GEN := glapi

include $(GLAPI)/sources.mak
GLAPI_OBJECTS := $(GLAPI_SOURCES:.c=.o)
GLAPI_SOURCES := $(addprefix $(GLAPI)/, $(GLAPI_SOURCES))
GLAPI_ASM_OBJECTS := $(GLAPI_ASM_SOURCES:.S=.o)
GLAPI_ASM_SOURCES := $(addprefix $(GEN)/, $(GLAPI_ASM_SOURCES))

include $(MAPI)/sources.mak
MAPI_GLAPI_OBJECTS := $(MAPI_GLAPI_SOURCES:.c=.o)
MAPI_GLAPI_SOURCES := $(addprefix $(MAPI)/, $(MAPI_GLAPI_SOURCES))

TARGET_OBJECTS = $(GLAPI_OBJECTS) $(GLAPI_ASM_OBJECTS) $(MAPI_GLAPI_OBJECTS)

INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mapi/$(TARGET) \
	-I$(TOP)/src/mapi \
	-I$(TOP)/src/mesa

.PHONY: default
default: depend lib$(TARGET).a

lib$(TARGET).a: $(TARGET_OBJECTS)
	@$(MKLIB) -o $(TARGET) -static $(TARGET_OBJECTS)

$(GLAPI_OBJECTS): %.o: $(GLAPI)/%.c
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@

$(GLAPI_ASM_OBJECTS): %.o: $(GEN)/%.S
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@

$(MAPI_GLAPI_OBJECTS): %.o: $(MAPI)/%.c
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) -DMAPI_GLAPI_CURRENT $< -o $@

$(GLAPI_SOURCES) $(GLAPI_ASM_SOURCES): | glapi-stamp

glapi-stamp:
	@# generate sources/headers
	@$(MAKE) -C $(GLAPI)/gen-es $(ES)
	@touch $@

.PHONY: clean
clean:
	-rm -f lib$(TARGET).a
	-rm -f $(TARGET_OBJECTS)
	-rm -f depend depend.bak
	-rm -f glapi-stamp
	@# clean generated sources/headers
	@$(MAKE) -C $(GLAPI)/gen-es clean-$(ES)

# nothing to install
install:

depend: $(GLAPI_SOURCES) $(MAPI_GLAPI_SOURCES)
	@echo "running $(MKDEP)"
	@touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) -f- $(DEFINES) $(INCLUDE_DIRS) \
		-DMAPI_GLAPI_CURRENT $(GLAPI_SOURCES) $(MAPI_GLAPI_SOURCES) \
		2>/dev/null | sed -e 's,^$(GLAPI)/,,' -e 's,^$(MAPI)/,,' \
		> depend
