# src/mapi/es1api/Makefile

TOP := ../../..
include $(TOP)/configs/current

# this Makefile can build both libes1api.a and libes2api.a
ifeq ($(ES),)
ES := es1
endif

ESAPI = $(ES)api

GLAPI := ../glapi
include $(GLAPI)/sources.mak
ESAPI_SOURCES := $(addprefix $(GLAPI)/, $(GLAPI_SOURCES))
ESAPI_OBJECTS := $(GLAPI_SOURCES:.c=.o)
ESAPI_ASM_SOURCES := $(addprefix glapi/, $(GLAPI_ASM_SOURCES))
ESAPI_ASM_OBJECTS := $(GLAPI_ASM_SOURCES:.S=.o)

INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mapi/$(ESAPI) \
	-I$(TOP)/src/mapi \
	-I$(TOP)/src/mesa

.PHONY: default
default: depend lib$(ESAPI).a

lib$(ESAPI).a: $(ESAPI_OBJECTS) $(ESAPI_ASM_OBJECTS)
	@$(MKLIB) -o $(ESAPI) -static $(ESAPI_OBJECTS) $(ESAPI_ASM_OBJECTS)

$(ESAPI_OBJECTS): %.o: $(GLAPI)/%.c
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@

$(ESAPI_ASM_OBJECTS): %.o: glapi/%.S
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@

$(ESAPI_SOURCES) $(ESAPI_ASM_SOURCES): | glapi-stamp

glapi-stamp:
	@$(MAKE) -C $(GLAPI)/gen-es $(ES)
	@touch $@

.PHONY: clean
clean:
	-rm -f $(ESAPI_OBJECTS) $(ESAPI_ASM_OBJECTS)
	-rm -f lib$(ESAPI).a
	-rm -f depend depend.bak
	@$(MAKE) -C $(GLAPI)/gen-es clean-$(ES)
	-rm -f glapi-stamp

# nothing to install
install:

depend: $(ESAPI_SOURCES)
	@echo "running $(MKDEP)"
	@touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) -f- $(DEFINES) $(INCLUDE_DIRS) \
		$(ESAPI_SOURCES) 2>/dev/null | \
		sed -e 's,^$(GLAPI)/,,' > depend
