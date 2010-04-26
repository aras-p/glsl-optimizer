# src/mapi/vgapi/Makefile

TOP := ../../..
include $(TOP)/configs/current

MAPI := $(TOP)/src/mapi/mapi

include $(MAPI)/sources.mak
VGAPI_SOURCES := $(addprefix $(MAPI)/, $(MAPI_SOURCES))
VGAPI_OBJECTS := $(MAPI_SOURCES:.c=.o)

VGAPI_CPPFLAGS := -DMAPI_ABI_HEADER=\"vgapi/vgapi_tmp.h\"

GENERATED_SOURCES := vgapi_tmp.h

INCLUDE_DIRS := \
	-I$(TOP)/include \
	-I$(TOP)/src/mapi

.PHONY: default
default: depend libvgapi.a

libvgapi.a: $(VGAPI_OBJECTS)
	@$(MKLIB) -o vgapi -static $(VGAPI_OBJECTS)

$(VGAPI_SOURCES): | $(GENERATED_SOURCES)

$(VGAPI_OBJECTS): %.o: $(MAPI)/%.c
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(VGAPI_CPPFLAGS) $< -o $@

vgapi_tmp.h: vgapi.csv $(MAPI)/mapi_abi.py
	$(PYTHON2) $(PYTHON_FLAGS) $(MAPI)/mapi_abi.py \
		-i vgapi/vgapi_defines.h $< > $@

.PHONY: clean
clean:
	-rm -f libvgapi.a
	-rm -f $(VGAPI_OBJECTS)
	-rm -f depend depend.bak
	-rm -f $(GENERATED_SOURCES)

# nothing to install
install:

depend: $(VGAPI_SOURCES)
	@echo "running $(MKDEP)"
	@touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) -f- $(DEFINES) $(INCLUDE_DIRS) \
		$(VGAPI_CPPFLAGS) $(VGAPI_SOURCES) 2>/dev/null | \
		sed -e 's,^$(MAPI)/,,' > depend
