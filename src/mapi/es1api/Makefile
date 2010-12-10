# src/mapi/es1api/Makefile

TOP := ../../..
include $(TOP)/configs/current

# this Makefile can build both OpenGL ES 1.1 and 2.0
ifeq ($(ES),)
ES := es1

GLES_HEADER := GLES
GLES_PC := glesv1_cm.pc

GLES_LIB := $(GLESv1_CM_LIB)
GLES_LIB_NAME := $(GLESv1_CM_LIB_NAME)
GLES_LIB_GLOB := $(GLESv1_CM_LIB_GLOB)
GLES_LIB_DEPS := $(GLESv1_CM_LIB_DEPS)
GLES_LIB_MAJOR := 1
GLES_LIB_MINOR := 1
GLES_LIB_PATCH := 0
else
GLES_HEADER := GLES2
GLES_PC := glesv2.pc

GLES_LIB := $(GLESv2_LIB)
GLES_LIB_NAME := $(GLESv2_LIB_NAME)
GLES_LIB_GLOB := $(GLESv2_LIB_GLOB)
GLES_LIB_DEPS := $(GLESv2_LIB_DEPS)
GLES_LIB_MAJOR := 2
GLES_LIB_MINOR := 0
GLES_LIB_PATCH := 0
endif

ESAPI = $(ES)api

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

ESAPI_OBJECTS = $(GLAPI_OBJECTS) $(GLAPI_ASM_OBJECTS) $(MAPI_GLAPI_OBJECTS)

INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mapi/$(ESAPI) \
	-I$(TOP)/src/mapi \
	-I$(TOP)/src/mesa

.PHONY: default
default: depend $(TOP)/$(LIB_DIR)/$(GLES_LIB_NAME)

$(TOP)/$(LIB_DIR)/$(GLES_LIB_NAME): $(ESAPI_OBJECTS)
	$(MKLIB) -o $(GLES_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(GLES_LIB_MAJOR) \
		-minor $(GLES_LIB_MINOR) \
		-patch $(GLES_LIB_PATCH) \
		-id $(INSTALL_LIB_DIR)/lib$(GLES_LIB).$(GLES_LIB_MAJOR).dylib \
		$(MKLIB_OPTIONS) -install $(TOP)/$(LIB_DIR) \
		$(ESAPI_OBJECTS) $(GLES_LIB_DEPS)

# not used, but kept for completeness
lib$(ESAPI).a: $(ESAPI_OBJECTS)
	@$(MKLIB) -o $(ESAPI) -static $(ESAPI_OBJECTS)

$(GLAPI_OBJECTS): %.o: $(GLAPI)/%.c
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) -DMAPI_GLAPI_CURRENT $< -o $@

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
	-rm -f $(GLES_PC)
	-rm -f $(TOP)/$(LIB_DIR)/$(GLES_LIB_NAME)
	-rm -f lib$(ESAPI).a
	-rm -f $(ESAPI_OBJECTS)
	-rm -f depend depend.bak
	-rm -f glapi-stamp
	@# clean generated sources/headers
	@$(MAKE) -C $(GLAPI)/gen-es clean-$(ES)

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

$(GLES_PC): $(GLES_PC).in
	@sed $(pcedit-$(ES)) $< > $@

install-headers:
	$(INSTALL) -d $(DESTDIR)$(INSTALL_INC_DIR)/$(GLES_HEADER)
	$(INSTALL) -m 644 $(TOP)/include/$(GLES_HEADER)/*.h \
		$(DESTDIR)$(INSTALL_INC_DIR)/$(GLES_HEADER)

install-pc: $(GLES_PC)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig
	$(INSTALL) -m 644 $(GLES_PC) $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig

install: default install-headers install-pc
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(MINSTALL) $(TOP)/$(LIB_DIR)/$(GLES_LIB_GLOB) \
		$(DESTDIR)$(INSTALL_LIB_DIR)

depend: $(GLAPI_SOURCES) $(MAPI_GLAPI_SOURCES)
	@echo "running $(MKDEP)"
	@touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) -f- $(DEFINES) $(INCLUDE_DIRS) \
		-DMAPI_GLAPI_CURRENT $(GLAPI_SOURCES) $(MAPI_GLAPI_SOURCES) \
		2>/dev/null | sed -e 's,^$(GLAPI)/,,' -e 's,^$(MAPI)/,,' \
		> depend
