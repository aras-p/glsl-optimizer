# src/gallium/targets/egl/Makefile
#
# This is the Makefile for EGL Gallium driver package.  The package consists of
#
#   egl_gallium.so - EGL driver
#   pipe_<HW>.so   - pipe drivers
#   st_<API>.so    - client API state trackers
#
# The following variables are examined
#
#   EGL_PLATFORMS       - platforms to support
#   GALLIUM_WINSYS_DIRS - pipe drivers to support
#   EGL_CLIENT_APIS     - state trackers to support
#

TOP = ../../../..
include $(TOP)/configs/current

ST_PREFIX := st_
PIPE_PREFIX := pipe_

common_CPPFLAGS := \
	-I$(TOP)/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/winsys \
	$(LIBDRM_CFLAGS)

common_SYS :=
common_LIBS := \
	$(TOP)/src/gallium/drivers/identity/libidentity.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(GALLIUM_AUXILIARIES)

# EGL driver
egl_CPPFLAGS := \
	-I$(TOP)/src/gallium/state_trackers/egl \
	-I$(TOP)/src/egl/main \
	-DPIPE_PREFIX=\"$(PIPE_PREFIX)\" -DST_PREFIX=\"$(ST_PREFIX)\"
egl_SYS := -lm $(DLOPEN_LIBS) -lEGL
egl_LIBS := $(TOP)/src/gallium/state_trackers/egl/libegl.a

ifneq ($(findstring x11, $(EGL_PLATFORMS)),)
egl_SYS += -lX11 -lXext -lXfixes $(LIBDRM_LIB)
egl_LIBS += $(TOP)/src/gallium/winsys/sw/xlib/libws_xlib.a
endif
ifneq ($(findstring drm, $(EGL_PLATFORMS)),)
egl_SYS += $(LIBDRM_LIB)
endif
ifneq ($(findstring fbdev, $(EGL_PLATFORMS)),)
egl_LIBS += $(TOP)/src/gallium/winsys/sw/fbdev/libfbdev.a
endif

# EGL_RENDERABLE_TYPE is a compile time attribute
ifneq ($(filter $(GL_LIB), $(EGL_CLIENT_APIS)),)
egl_CPPFLAGS += $(API_DEFINES)
endif
ifneq ($(filter $(VG_LIB), $(EGL_CLIENT_APIS)),)
egl_CPPFLAGS += -DFEATURE_VG=1
endif
egl_CPPFLAGS := $(sort $(egl_CPPFLAGS))

# i915 pipe driver
i915_CPPFLAGS :=
i915_SYS := -ldrm_intel
i915_LIBS := \
	$(TOP)/src/gallium/winsys/i915/drm/libi915drm.a \
	$(TOP)/src/gallium/drivers/i915/libi915.a

# i965 pipe driver
i965_CPPFLAGS :=
i965_SYS := -ldrm_intel
i965_LIBS := \
	$(TOP)/src/gallium/winsys/i965/drm/libi965drm.a \
	$(TOP)/src/gallium/drivers/i965/libi965.a \
	$(TOP)/src/gallium/winsys/sw/wrapper/libwsw.a

# nouveau pipe driver
nouveau_CPPFLAGS :=
nouveau_SYS := -ldrm_nouveau
nouveau_LIBS := \
	$(TOP)/src/gallium/winsys/nouveau/drm/libnouveaudrm.a \
	$(TOP)/src/gallium/drivers/nvfx/libnvfx.a \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a \
	$(TOP)/src/gallium/drivers/nvc0/libnvc0.a \
	$(TOP)/src/gallium/drivers/nouveau/libnouveau.a

# r300 pipe driver
r300_CPPFLAGS :=
r300_SYS := -ldrm -ldrm_radeon
r300_LIBS := \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/r300/libr300.a

# r600 pipe driver
r600_CPPFLAGS :=
r600_SYS := -ldrm -ldrm_radeon
r600_LIBS := \
	$(TOP)/src/gallium/winsys/r600/drm/libr600winsys.a \
	$(TOP)/src/gallium/drivers/r600/libr600.a

# vmwgfx pipe driver
vmwgfx_CPPFLAGS :=
vmwgfx_SYS :=
vmwgfx_LIBS := \
	$(TOP)/src/gallium/winsys/svga/drm/libsvgadrm.a \
	$(TOP)/src/gallium/drivers/svga/libsvga.a

# swrast (pseudo) pipe driver
swrast_CPPFLAGS := -DGALLIUM_SOFTPIPE -DGALLIUM_RBUG -DGALLIUM_TRACE
swrast_SYS := -lm
swrast_LIBS := $(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a

# LLVM
ifeq ($(MESA_LLVM),1)
common_SYS += $(LLVM_LIBS)
swrast_CPPFLAGS += -DGALLIUM_LLVMPIPE
swrast_LIBS += $(TOP)/src/gallium/drivers/llvmpipe/libllvmpipe.a
LDFLAGS += $(LLVM_LDFLAGS)
endif

# OpenGL state tracker
GL_CPPFLAGS := -I$(TOP)/src/mesa $(API_DEFINES)
# cannot not link to $(GL_LIB) as the app might want GLES
GL_SYS := $(DRI_LIB_DEPS)
GL_LIBS := $(TOP)/src/mesa/libmesagallium.a

# OpenVG state tracker
OpenVG_CPPFLAGS := -I$(TOP)/src/gallium/state_trackers/vega
OpenVG_SYS := -lm -l$(VG_LIB)
OpenVG_LIBS := $(TOP)/src/gallium/state_trackers/vega/libvega.a


OUTPUT_PATH := $(TOP)/$(LIB_DIR)/egl

# determine the outputs
ifneq ($(findstring i915/drm,$(GALLIUM_WINSYS_DIRS)),)
OUTPUTS += i915
endif
ifneq ($(findstring i965/drm,$(GALLIUM_WINSYS_DIRS)),)
OUTPUTS += i965
endif
ifneq ($(findstring nouveau/drm,$(GALLIUM_WINSYS_DIRS)),)
OUTPUTS += nouveau
endif
ifneq ($(findstring radeon/drm,$(GALLIUM_WINSYS_DIRS)),)
OUTPUTS += r300
endif
ifneq ($(findstring r600/drm,$(GALLIUM_WINSYS_DIRS)),)
OUTPUTS += r600
endif
ifneq ($(findstring svga/drm,$(GALLIUM_WINSYS_DIRS)),)
OUTPUTS += vmwgfx
endif
OUTPUTS += swrast
OUTPUTS := $(addprefix $(PIPE_PREFIX), $(OUTPUTS))

# EGL driver and state trackers
OUTPUTS += egl_gallium $(addprefix $(ST_PREFIX), $(EGL_CLIENT_APIS))

OUTPUTS := $(addsuffix .so, $(OUTPUTS))
OUTPUTS := $(addprefix $(OUTPUT_PATH)/, $(OUTPUTS))

default: $(OUTPUTS)

define mklib
$(MKLIB) -o $(notdir $@) -noprefix -linker '$(CC)' \
	-L$(TOP)/$(LIB_DIR) -ldflags '$(LDFLAGS)' \
	-install $(OUTPUT_PATH) $(MKLIB_OPTIONS) $< \
	-Wl,--start-group $(common_LIBS) $($(1)_LIBS) -Wl,--end-group \
	$(common_SYS) $($(1)_SYS)
endef

define mklib-cxx
$(MKLIB) -o $(notdir $@) -noprefix -linker '$(CXX)' \
	-L$(TOP)/$(LIB_DIR) -ldflags '$(LDFLAGS)' \
	-cplusplus -install $(OUTPUT_PATH) $(MKLIB_OPTIONS) $< \
	-Wl,--start-group $(common_LIBS) $($(1)_LIBS) -Wl,--end-group \
	$(common_SYS) $($(1)_SYS)
endef

# EGL driver
$(OUTPUT_PATH)/egl_gallium.so: egl.o $(egl_LIBS)
	$(call mklib,egl)

# pipe drivers
$(OUTPUT_PATH)/$(PIPE_PREFIX)i915.so: pipe_i915.o $(i915_LIBS)
	$(call mklib,i915)

$(OUTPUT_PATH)/$(PIPE_PREFIX)i965.so: pipe_i965.o $(i965_LIBS)
	$(call mklib,i965)

$(OUTPUT_PATH)/$(PIPE_PREFIX)nouveau.so: pipe_nouveau.o $(nouveau_LIBS)
	$(call mklib,nouveau)

$(OUTPUT_PATH)/$(PIPE_PREFIX)r300.so: pipe_r300.o $(r300_LIBS)
	$(call mklib,r300)

$(OUTPUT_PATH)/$(PIPE_PREFIX)r600.so: pipe_r600.o $(r600_LIBS)
	$(call mklib,r600)

$(OUTPUT_PATH)/$(PIPE_PREFIX)vmwgfx.so: pipe_vmwgfx.o $(vmwgfx_LIBS)
	$(call mklib,vmwgfx)

$(OUTPUT_PATH)/$(PIPE_PREFIX)swrast.so: pipe_swrast.o $(swrast_LIBS)
	$(call mklib,swrast)

# state trackers
$(OUTPUT_PATH)/$(ST_PREFIX)$(GL_LIB).so: st_GL.o $(GL_LIBS)
	$(call mklib-cxx,GL)

$(OUTPUT_PATH)/$(ST_PREFIX)$(VG_LIB).so: st_OpenVG.o $(OpenVG_LIBS)
	$(call mklib,OpenVG)

egl.o: egl.c
	$(CC) -c -o $@ $< $(common_CPPFLAGS) $(egl_CPPFLAGS) $(DEFINES) $(CFLAGS)

pipe_%.o: pipe_%.c
	$(CC) -c -o $@ $< $(common_CPPFLAGS) $($*_CPPFLAGS) $(DEFINES) $(CFLAGS)

st_%.o: st_%.c
	$(CC) -c -o $@ $< $(common_CPPFLAGS) $($*_CPPFLAGS) $(DEFINES) $(CFLAGS)

install: $(OUTPUTS)
	$(INSTALL) -d $(DESTDIR)$(EGL_DRIVER_INSTALL_DIR)
	for out in $(OUTPUTS); do \
		$(MINSTALL) -m 755 "$$out" $(DESTDIR)$(EGL_DRIVER_INSTALL_DIR); \
	done

clean:
	rm -f *.o
