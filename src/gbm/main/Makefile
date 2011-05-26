# src/gbm/main/Makefile

TOP = ../../..
include $(TOP)/configs/current

INCLUDE_DIRS = -I$(TOP)/include

HEADERS =		\
	common.h	\
	backend.h	\
	gbmint.h	\
	gbm.h	

SOURCES =		\
	gbm.c		\
	backend.c	\
	common.c

OBJECTS = $(SOURCES:.c=.o)

# use dl*() to load drivers
LOCAL_CFLAGS = $(LIBUDEV_CFLAGS) $(DLOPEN_CFLAGS) \
	       -D_OS_UNIX=1 -DMODULEDIR='"$(GBM_BACKEND_INSTALL_DIR)"'
LOCAL_LIBS =

# Builtin backends
ifeq ($(filter dri, $(GBM_BACKEND_DIRS)),dri)
LOCAL_LIBS += $(TOP)/src/gbm/backends/dri/libgbm_dri.a
endif

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(LOCAL_CFLAGS) $< -o $@


default: depend library


library: $(TOP)/$(LIB_DIR)/libgbm.so

$(TOP)/$(LIB_DIR)/libgbm.so: $(OBJECTS) $(LOCAL_LIBS)
	$(MKLIB) -o gbm -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major 1 -minor 0 \
		-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
		-L$(TOP)/$(LIB_DIR) $(GBM_LIB_DEPS) \
		$(OBJECTS) $(LOCAL_LIBS)

install-headers:
	$(INSTALL) -d $(DESTDIR)$(INSTALL_INC_DIR)/
	$(INSTALL) -m 644 $(TOP)/src/gbm/main/gbm.h \
		$(DESTDIR)$(INSTALL_INC_DIR)


PKG_CONFIG_DIR = $(INSTALL_LIB_DIR)/pkgconfig

gbm_pcedit = sed \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),' \
	-e 's,@INSTALL_LIB_DIR@,$(INSTALL_LIB_DIR),' \
	-e 's,@INSTALL_INC_DIR@,$(INSTALL_INC_DIR),' \
	-e 's,@VERSION@,0.0.0,' \
	-e 's,@GBM_PC_REQ_PRIV@,$(GBM_PC_REQ_PRIV),' \
	-e 's,@GBM_PC_LIB_PRIV@,$(GBM_PC_LIB_PRIV),' \
	-e 's,@GBM_PC_CFLAGS@,$(GBM_PC_CFLAGS),' \
	-e 's,@GBM_LIB@,$(GBM_LIB),'

gbm.pc: gbm.pc.in
	$(gbm_pcedit) $< > $@

install: default install-headers gbm.pc
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(MINSTALL) $(TOP)/$(LIB_DIR)/libgbm.so* \
		$(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(PKG_CONFIG_DIR)
	$(INSTALL) -m 644 gbm.pc $(DESTDIR)$(PKG_CONFIG_DIR)

clean:
	-rm -f *.o
	-rm -f depend depend.bak
	-rm -f gbm.pc


depend: $(SOURCES) $(HEADERS)
	@ echo "running $(MKDEP)"
	@ rm -f depend
	@ touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) \
		$(SOURCES) $(HEADERS) > /dev/null 2>/dev/null


-include depend
# DO NOT DELETE
