# src/mapi/vgapi/Makefile

TOP := ../../..
include $(TOP)/configs/current

VG_LIB_MAJOR = 1
VG_LIB_MINOR = 0
VG_LIB_PATCH = 0

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
default: depend $(TOP)/$(LIB_DIR)/$(VG_LIB_NAME)

$(TOP)/$(LIB_DIR)/$(VG_LIB_NAME): $(VGAPI_OBJECTS)
	$(MKLIB) -o $(VG_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(VG_LIB_MAJOR) \
		-minor $(VG_LIB_MINOR) \
		-patch $(VG_LIB_PATCH) \
		-id $(INSTALL_LIB_DIR)/lib$(VG_LIB).$(VG_LIB_MAJOR).dylib \
		$(MKLIB_OPTIONS) -install $(TOP)/$(LIB_DIR) \
		$(VGAPI_OBJECTS) $(VG_LIB_DEPS)

# not used, but kept for completeness
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
	-rm -f $(TOP)/$(LIB_DIR)/$(VG_LIB_NAME)
	-rm -f libvgapi.a
	-rm -f $(VGAPI_OBJECTS)
	-rm -f depend depend.bak
	-rm -f $(GENERATED_SOURCES)

install-headers:
	$(INSTALL) -d $(DESTDIR)$(INSTALL_INC_DIR)/VG
	$(INSTALL) -m 644 $(TOP)/include/VG/*.h $(DESTDIR)$(INSTALL_INC_DIR)/VG

install: default install-headers
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(MINSTALL) $(TOP)/$(LIB_DIR)/$(VG_LIB_GLOB) \
		$(DESTDIR)$(INSTALL_LIB_DIR)

depend: $(VGAPI_SOURCES)
	@echo "running $(MKDEP)"
	@touch depend
	@$(MKDEP) $(MKDEP_OPTIONS) -f- $(DEFINES) $(INCLUDE_DIRS) \
		$(VGAPI_CPPFLAGS) $(VGAPI_SOURCES) 2>/dev/null | \
		sed -e 's,^$(MAPI)/,,' > depend
