# This makefile produces a libXvMCg3dvl.so which is
# based on DRM/DRI

TOP = ../../../../..
include $(TOP)/configs/current

XVMC_MAJOR = 1
XVMC_MINOR = 0
XVMC_LIB = XvMCg3dvl
XVMC_LIB_NAME = lib$(XVMC_LIB).so
XVMC_LIB_DEPS = $(EXTRA_LIB_PATH) -lXvMC -lXv -lX11 -lm

SOURCES = drm_nouveau_winsys.c

DRIVER_INCLUDES = $(shell pkg-config libdrm libdrm_nouveau --cflags-only-I) \
                   -I$(TOP)/src/gallium/winsys/drm/nouveau \
                   -I$(TOP)/src/driclient/include
DRIVER_DEFINES = $(shell pkg-config libdrm libdrm_nouveau --cflags-only-other)

PIPE_DRIVERS = \
	$(TOP)/src/gallium/winsys/drm/nouveau/drm/libnouveaudrm.a \
	$(TOP)/src/gallium/drivers/nv04/libnv04.a \
	$(TOP)/src/gallium/drivers/nv10/libnv10.a \
	$(TOP)/src/gallium/drivers/nv20/libnv20.a \
	$(TOP)/src/gallium/drivers/nv30/libnv30.a \
	$(TOP)/src/gallium/drivers/nv40/libnv40.a \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a \
	$(TOP)/src/gallium/drivers/nouveau/libnouveau.a

DRI_LIB_DEPS += $(shell pkg-config libdrm_nouveau --libs)

###############################################################

INCLUDES = $(DRIVER_INCLUDES) \
           -I$(TOP)/src/gallium/include \
           -I$(TOP)/src/gallium/auxiliary \
           -I$(TOP)/src/gallium/drivers \
           -I$(TOP)/src/gallium/winsys/g3dvl \
           -I$(TOP)/src/driclient/include

DEFINES += $(DRIVER_DEFINES) \
           -DGALLIUM_SOFTPIPE \
	   -DGALLIUM_TRACE

# XXX: Hack, if we include libxvmctracker.a in LIBS none of the symbols are
# pulled in by the linker because xsp_winsys.c doesn't refer to them
OBJECTS = $(SOURCES:.c=.o) $(TOP)/src/gallium/state_trackers/xorg/xvmc/*.o

LIBS = $(PIPE_DRIVERS) \
       $(TOP)/src/driclient/lib/libdriclient.a \
       $(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
       $(TOP)/src/gallium/auxiliary/vl/libvl.a \
       $(TOP)/src/gallium/auxiliary/draw/libdraw.a \
       $(TOP)/src/gallium/auxiliary/tgsi/libtgsi.a \
       $(TOP)/src/gallium/auxiliary/translate/libtranslate.a \
       $(TOP)/src/gallium/auxiliary/cso_cache/libcso_cache.a \
       $(TOP)/src/gallium/auxiliary/rtasm/librtasm.a \
       $(TOP)/src/gallium/auxiliary/util/libutil.a

.c.o:
	$(CC) -c $(INCLUDES) $(DEFINES) $(CFLAGS) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(DEFINES) $(CFLAGS) $< -o $@

.PHONY: default $(TOP)/$(LIB_DIR)/gallium clean

default: depend $(TOP)/$(LIB_DIR)/gallium $(TOP)/$(LIB_DIR)/gallium/$(XVMC_LIB_NAME)

$(TOP)/$(LIB_DIR)/gallium:
	@mkdir -p $(TOP)/$(LIB_DIR)/gallium

# Make the libXvMCg3dvl.so library
$(TOP)/$(LIB_DIR)/gallium/$(XVMC_LIB_NAME): $(OBJECTS) $(LIBS) Makefile
	$(MKLIB) -o $(XVMC_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(XVMC_MAJOR) -minor $(XVMC_MINOR) $(MKLIB_OPTIONS) \
		-install $(TOP)/$(LIB_DIR)/gallium -id $(INSTALL_LIB_DIR)/lib$(XVMC_LIB).1.dylib \
		$(XVMC_LIB_DEPS) $(DRI_LIB_DEPS) $(OBJECTS) $(LIBS)

depend: $(SOURCES) Makefile
	$(RM) depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDES) $(SOURCES)

#install: default
#	$(INSTALL) -d $(INSTALL_DIR)/include/GL
#	$(INSTALL) -d $(INSTALL_DIR)/$(LIB_DIR)
#	$(INSTALL) -m 644 $(TOP)/include/GL/*.h $(INSTALL_DIR)/include/GL
#	@if [ -e $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) ]; then \
#		$(INSTALL) $(TOP)/$(LIB_DIR)/libGL* $(INSTALL_DIR)/$(LIB_DIR); \
#	fi

clean: Makefile
	$(RM) $(TOP)/$(LIB_DIR)/gallium/$(XVMC_LIB_NAME)
	$(RM) *.o *~
	$(RM) depend depend.bak

-include depend
