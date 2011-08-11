# src/mapi/es1api/Makefile

TOP := ../../..
include $(TOP)/configs/current

# this Makefile can build both OpenGL ES 1.1 and 2.0
ifeq ($(ES),)
ES := es1

esapi_HEADER := GLES
esapi_PC := glesv1_cm.pc

esapi_LIB := $(GLESv1_CM_LIB)
esapi_LIB_NAME := $(GLESv1_CM_LIB_NAME)
esapi_LIB_GLOB := $(GLESv1_CM_LIB_GLOB)
esapi_LIB_DEPS := $(GLESv1_CM_LIB_DEPS)
esapi_LIB_MAJOR := 1
esapi_LIB_MINOR := 1
esapi_LIB_PATCH := 0
else
esapi_HEADER := GLES2
esapi_PC := glesv2.pc

esapi_LIB := $(GLESv2_LIB)
esapi_LIB_NAME := $(GLESv2_LIB_NAME)
esapi_LIB_GLOB := $(GLESv2_LIB_GLOB)
esapi_LIB_DEPS := $(GLESv2_LIB_DEPS)
esapi_LIB_MAJOR := 2
esapi_LIB_MINOR := 0
esapi_LIB_PATCH := 0
endif

ESAPI = $(ES)api

GLAPI := $(TOP)/src/mapi/glapi
MAPI := $(TOP)/src/mapi/mapi

esapi_CPPFLAGS := \
	-I$(TOP)/include \
	-I$(TOP)/src/mapi \
	-DMAPI_ABI_HEADER=\"$(ESAPI)/glapi_mapi_tmp.h\"

include $(MAPI)/sources.mak
esapi_SOURCES := $(addprefix $(MAPI)/, $(MAPI_BRIDGE_SOURCES))
esapi_OBJECTS := $(MAPI_BRIDGE_SOURCES:.c=.o)
esapi_CPPFLAGS += -DMAPI_MODE_BRIDGE

esapi_LIB_DEPS := -L$(TOP)/$(LIB_DIR) -l$(GLAPI_LIB) $(esapi_LIB_DEPS)

.PHONY: default
default: depend $(TOP)/$(LIB_DIR)/$(esapi_LIB_NAME)

$(TOP)/$(LIB_DIR)/$(esapi_LIB_NAME): $(esapi_OBJECTS)
	$(MKLIB) -o $(esapi_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(esapi_LIB_MAJOR) \
		-minor $(esapi_LIB_MINOR) \
		-patch $(esapi_LIB_PATCH) \
		-id $(INSTALL_LIB_DIR)/lib$(esapi_LIB).$(esapi_LIB_MAJOR).dylib \
		$(MKLIB_OPTIONS) -install $(TOP)/$(LIB_DIR) \
		$(esapi_OBJECTS) $(esapi_LIB_DEPS)

# not used, but kept for completeness
lib$(ESAPI).a: $(esapi_OBJECTS)
	@$(MKLIB) -o $(ESAPI) -static $(esapi_OBJECTS)

$(esapi_OBJECTS): %.o: $(MAPI)/%.c
	$(CC) -c $(esapi_CPPFLAGS) $(CFLAGS) $< -o $@

$(esapi_SOURCES): glapi_mapi_tmp.h

include $(GLAPI)/gen/glapi_gen.mk
glapi_mapi_tmp.h: $(GLAPI)/gen/gl_and_es_API.xml $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi,$<,$(ESAPI))

.PHONY: clean
clean:
	-rm -f $(esapi_PC)
	-rm -f $(TOP)/$(LIB_DIR)/$(esapi_LIB_NAME)
	-rm -f lib$(ESAPI).a
	-rm -f $(esapi_OBJECTS)
	-rm -f depend depend.bak
	-rm -f glapi_mapi_tmp.h

pcedit = \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),' \
	-e 's,@INSTALL_LIB_DIR@,$(INSTALL_LIB_DIR),' \
	-e 's,@INSTALL_INC_DIR@,$(INSTALL_INC_DIR),' \
	-e 's,@VERSION@,$(MESA_MAJOR).$(MESA_MINOR).$(MESA_TINY),' \

pcedit-es1 = \
	$(pcedit) \
	-e 's,@GLESv1_CM_PC_REQ_PRIV@,$(GLESv1_CM_PC_REQ_PRIV),' \
	-e 's,@GLESv1_CM_PC_LIB_PRIV@,$(GLESv1_CM_PC_LIB_PRIV),' \
	-e 's,@GLESv1_CM_PC_CFLAGS@,$(GLESv1_CM_PC_CFLAGS),' \
	-e 's,@GLESv1_CM_LIB@,$(GLESv1_CM_LIB),'

pcedit-es2 = \
	$(pcedit) \
	-e 's,@GLESv2_PC_REQ_PRIV@,$(GLESv2_PC_REQ_PRIV),' \
	-e 's,@GLESv2_PC_LIB_PRIV@,$(GLESv2_PC_LIB_PRIV),' \
	-e 's,@GLESv2_PC_CFLAGS@,$(GLESv2_PC_CFLAGS),' \
	-e 's,@GLESv2_LIB@,$(GLESv2_LIB),'

$(esapi_PC): $(esapi_PC).in
	@sed $(pcedit-$(ES)) $< > $@

install-headers:
	$(INSTALL) -d $(DESTDIR)$(INSTALL_INC_DIR)/$(esapi_HEADER)
	$(INSTALL) -m 644 $(TOP)/include/$(esapi_HEADER)/*.h \
		$(DESTDIR)$(INSTALL_INC_DIR)/$(esapi_HEADER)

install-pc: $(esapi_PC)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig
	$(INSTALL) -m 644 $(esapi_PC) $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig

install: default install-headers install-pc
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(MINSTALL) $(TOP)/$(LIB_DIR)/$(esapi_LIB_GLOB) \
		$(DESTDIR)$(INSTALL_LIB_DIR)

# workaround a bug in makedepend
makedepend_CPPFLAGS := \
	$(filter-out -DMAPI_ABI_HEADER=%, $(esapi_CPPFLAGS))
$(esapi_OBJECTS): glapi_mapi_tmp.h

depend: $(esapi_SOURCES)
	@echo "running $(MKDEP)"
	@touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) -f- $(DEFINES) $(makedepend_CPPFLAGS) \
		$(esapi_SOURCES) 2>/dev/null | \
		sed -e 's,^$(GLAPI)/,,' -e 's,^$(MAPI)/,,' > depend

-include depend
