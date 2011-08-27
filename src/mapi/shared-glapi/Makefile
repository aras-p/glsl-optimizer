# src/mapi/shared-glapi/Makefile
#
# Used by OpenGL ES or when --enable-shared-glapi is specified
#

TOP := ../../..
include $(TOP)/configs/current

GLAPI := $(TOP)/src/mapi/glapi
MAPI := $(TOP)/src/mapi/mapi

glapi_CPPFLAGS := \
	-I$(TOP)/include \
	-I$(TOP)/src/mapi \
	-DMAPI_MODE_GLAPI \
	-DMAPI_ABI_HEADER=\"shared-glapi/glapi_mapi_tmp.h\"

include $(MAPI)/sources.mak
glapi_SOURCES := $(addprefix $(MAPI)/, $(MAPI_GLAPI_SOURCES))
glapi_OBJECTS := $(MAPI_GLAPI_SOURCES:.c=.o)

.PHONY: default
default: depend $(TOP)/$(LIB_DIR)/$(GLAPI_LIB_NAME)

$(TOP)/$(LIB_DIR)/$(GLAPI_LIB_NAME): $(glapi_OBJECTS)
	$(MKLIB) -o $(GLAPI_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major 0 -minor 0 -patch 0 \
		-id $(INSTALL_LIB_DIR)/lib$(GLAPI_LIB).0.dylib \
		$(MKLIB_OPTIONS) -install $(TOP)/$(LIB_DIR) \
		$(glapi_OBJECTS) $(GLAPI_LIB_DEPS)

$(glapi_OBJECTS): %.o: $(MAPI)/%.c
	$(CC) -c $(glapi_CPPFLAGS) $(CFLAGS) $< -o $@

$(glapi_SOURCES): glapi_mapi_tmp.h

include $(GLAPI)/gen/glapi_gen.mk
glapi_mapi_tmp.h: $(GLAPI)/gen/gl_and_es_API.xml $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi,$<,shared-glapi)

.PHONY: clean
clean:
	-rm -f $(TOP)/$(LIB_DIR)/$(GLAPI_LIB_NAME)
	-rm -f $(glapi_OBJECTS)
	-rm -f depend depend.bak
	-rm -f glapi_mapi_tmp.h

install:
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(MINSTALL) $(TOP)/$(LIB_DIR)/$(GLAPI_LIB_GLOB) \
		$(DESTDIR)$(INSTALL_LIB_DIR)

# workaround a bug in makedepend
makedepend_CPPFLAGS := \
	$(filter-out -DMAPI_ABI_HEADER=%, $(glapi_CPPFLAGS))
$(glapi_OBJECTS): glapi_mapi_tmp.h

depend: $(glapi_SOURCES)
	@echo "running $(MKDEP)"
	@touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) -f- $(DEFINES) $(makedepend_CPPFLAGS) \
		$(glapi_SOURCES) 2>/dev/null | sed -e 's,^$(MAPI)/,,' \
		> depend

-include depend
