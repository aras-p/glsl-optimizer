# src/gallium/targets/gbm/Makefile

TOP = ../../../..
include $(TOP)/configs/current

PIPE_PREFIX := pipe_

GBM_BACKEND = gbm_gallium_drm
GBM_SOURCES = gbm.c pipe_loader.c

GBM_INCLUDES = \
	       -I$(TOP)/include \
	       -I$(TOP)/src/gallium/state_trackers/gbm \
	       -I$(TOP)/src/gbm/main \
	       -I$(TOP)/src/gallium/auxiliary \
	       -I$(TOP)/src/gallium/include \

GBM_LIBS = $(LIBUDEV_LIBS) $(LIBDRM_LIB) -lm \
	   $(TOP)/src/gallium/state_trackers/gbm/libgbm.a \
	   $(TOP)/src/gallium/drivers/identity/libidentity.a \
	   $(TOP)/src/gallium/drivers/galahad/libgalahad.a \
	   $(TOP)/src/gallium/drivers/trace/libtrace.a \
	   $(TOP)/src/gallium/drivers/rbug/librbug.a \
	   $(GALLIUM_AUXILIARIES)


GBM_CFLAGS = \
	     -DGBM_BACKEND_SEARCH_DIR=\"$(GBM_BACKEND_INSTALL_DIR)\" \
	     -DPIPE_PREFIX=\"$(PIPE_PREFIX)\" \
	     $(LIBUDEV_CFLAGS) \
	     $(LIBDRM_CFLAGS)  


pipe_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/winsys

pipe_LIBS = \
	$(TOP)/src/gallium/drivers/identity/libidentity.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(GALLIUM_AUXILIARIES)

# as if we are DRI modules
pipe_SYS = $(DRI_LIB_DEPS)

pipe_CLFLAGS = \
	-DGALLIUM_RBUG -DGALLIUM_TRACE -DGALLIUM_GALAHAD \
	$(LIBDRM_CFLAGS)

pipe_LDFLAGS = -Wl,--no-undefined

# i915 pipe driver
i915_LIBS = \
	$(TOP)/src/gallium/winsys/i915/drm/libi915drm.a \
	$(TOP)/src/gallium/drivers/i915/libi915.a
i915_SYS = -ldrm_intel

# nouveau pipe driver
nouveau_LIBS = \
	$(TOP)/src/gallium/winsys/nouveau/drm/libnouveaudrm.a \
	$(TOP)/src/gallium/drivers/nvfx/libnvfx.a \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a \
	$(TOP)/src/gallium/drivers/nvc0/libnvc0.a \
	$(TOP)/src/gallium/drivers/nouveau/libnouveau.a
nouveau_SYS = -ldrm_nouveau

# r300 pipe driver
r300_LIBS = \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/r300/libr300.a

# r600 pipe driver
r600_LIBS = \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/r600/libr600.a

# vmwgfx pipe driver
vmwgfx_LIBS = \
	$(TOP)/src/gallium/winsys/svga/drm/libsvgadrm.a \
	$(TOP)/src/gallium/drivers/svga/libsvga.a



# LLVM
ifeq ($(MESA_LLVM),1)
pipe_SYS += $(LLVM_LIBS)
pipe_LDFLAGS += $(LLVM_LDFLAGS)
endif

ifneq ($(findstring llvmpipe,$(GALLIUM_DRIVERS_DIRS)),)
pipe_LIBS += $(TOP)/src/gallium/drivers/llvmpipe/libllvmpipe.a
endif

# determine the targets/sources
_pipe_TARGETS_CC =
_pipe_TARGETS_CXX =
pipe_SOURCES =

ifneq ($(findstring i915/drm,$(GALLIUM_WINSYS_DIRS)),)
_pipe_TARGETS_CC += $(PIPE_PREFIX)i915.so
pipe_SOURCES += pipe_i915.c
endif

ifneq ($(findstring nouveau/drm,$(GALLIUM_WINSYS_DIRS)),)
_pipe_TARGETS_CXX += $(PIPE_PREFIX)nouveau.so
pipe_SOURCES += pipe_nouveau.c
endif

ifneq ($(findstring radeon/drm,$(GALLIUM_WINSYS_DIRS)),)
ifneq ($(findstring r300,$(GALLIUM_DRIVERS_DIRS)),)
_pipe_TARGETS_CC += $(PIPE_PREFIX)r300.so
pipe_SOURCES += pipe_r300.c
endif
endif

ifneq ($(findstring radeon/drm,$(GALLIUM_WINSYS_DIRS)),)
ifneq ($(findstring r600,$(GALLIUM_DRIVERS_DIRS)),)
_pipe_TARGETS_CC += $(PIPE_PREFIX)r600.so
pipe_SOURCES += pipe_r600.c
endif
endif

ifneq ($(findstring svga/drm,$(GALLIUM_WINSYS_DIRS)),)
_pipe_TARGETS_CC += $(PIPE_PREFIX)vmwgfx.so
pipe_SOURCES += pipe_vmwgfx.c
endif

pipe_OBJECTS = $(pipe_SOURCES:.c=.o)

ifeq ($(MESA_LLVM),1)
pipe_TARGETS_CXX = $(_pipe_TARGETS_CXX) $(_pipe_TARGETS_CC)
pipe_TARGETS_CC =
else
pipe_TARGETS_CXX = $(_pipe_TARGETS_CXX)
pipe_TARGETS_CC = $(_pipe_TARGETS_CC)
endif

GBM_EXTRA_TARGETS = $(addprefix $(TOP)/$(LIB_DIR)/gbm/, $(pipe_TARGETS_CC)) $(addprefix $(TOP)/$(LIB_DIR)/gbm/, $(pipe_TARGETS_CXX))
GBM_EXTRA_INSTALL = install-pipes
GBM_EXTRA_CLEAN = clean-pipes
GBM_EXTRA_SOURCES = $(pipe_SOURCES)

include $(TOP)/src/gbm/backends/Makefile.template


$(GBM_EXTRA_TARGETS): $(TOP)/$(LIB_DIR)/gbm/%: %
	@$(INSTALL) -d $(dir $@)
	$(INSTALL) $< $(dir $@)

$(pipe_TARGETS_CC): $(PIPE_PREFIX)%.so: pipe_%.o $(pipe_LIBS) $($*_LIBS)
	$(MKLIB) -o $@ -noprefix -linker '$(CC)' \
		-ldflags '-L$(TOP)/$(LIB_DIR) $(pipe_LDFLAGS) $(LDFLAGS)' \
		$(MKLIB_OPTIONS) $< \
		-Wl,--start-group $(pipe_LIBS) $($*_LIBS) -Wl,--end-group \
		$(pipe_SYS) $($*_SYS)

$(pipe_TARGETS_CXX): $(PIPE_PREFIX)%.so: pipe_%.o $(pipe_LIBS) $($*_LIBS)
	$(MKLIB) -o $@ -noprefix -linker '$(CXX)' \
		-ldflags '-L$(TOP)/$(LIB_DIR) $(pipe_LDFLAGS) $(LDFLAGS)' \
		$(MKLIB_OPTIONS) $< \
		-Wl,--start-group $(pipe_LIBS) $($*_LIBS) -Wl,--end-group \
		$(pipe_SYS) $($*_SYS)

$(pipe_OBJECTS): %.o: %.c
	$(CC) -c -o $@ $< $(pipe_INCLUDES) $(pipe_CFLAGS) $(CFLAGS)

install-pipes: $(GBM_EXTRA_TARGETS)
	$(INSTALL) -d $(DESTDIR)$(GBM_BACKEND_INSTALL_DIR)
	for tgt in $(GBM_EXTRA_TARGETS); do \
		$(MINSTALL) "$$tgt" $(DESTDIR)$(GBM_BACKEND_INSTALL_DIR); \
	done

clean-pipes:
	rm -f $(pipe_TARGETS)
	rm -f $(pipe_OBJECTS)
