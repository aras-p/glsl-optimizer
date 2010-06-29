# src/gallium/targets/egl/Makefile
#
# This is the Makefile for EGL Gallium driver package.  The package consists of
#
#   egl_gallium_<HW>.so - EGL drivers
#   st_<API>.so         - client API state trackers
#
# The following variables are examined
#
#   EGL_PLATFORMS       - platforms to support
#   GALLIUM_WINSYS_DIRS - pipe drivers to support
#   EGL_CLIENT_APIS     - state trackers to support
#

TOP = ../../../..
include $(TOP)/configs/current

common_CPPFLAGS := \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/winsys
common_SYS :=
common_LIBS := \
	$(TOP)/src/gallium/drivers/identity/libidentity.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(GALLIUM_AUXILIARIES)

# EGL driver
egl_CPPFLAGS := \
	-I$(TOP)/src/gallium/state_trackers/egl \
	-I$(TOP)/src/egl/main
egl_SYS := -lm -ldl -lEGL
egl_LIBS := \
	$(TOP)/src/gallium/state_trackers/egl/libegl.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a

ifneq ($(findstring x11, $(EGL_PLATFORMS)),)
egl_SYS += -lX11 -lXext -lXfixes
egl_LIBS += $(TOP)/src/gallium/winsys/sw/xlib/libws_xlib.a
endif
ifneq ($(findstring kms, $(EGL_PLATFORMS)),)
egl_SYS += -ldrm
endif
ifneq ($(findstring fbdev, $(EGL_PLATFORMS)),)
egl_LIBS += $(TOP)/src/gallium/winsys/sw/fbdev/libfbdev.a
endif

# LLVM
ifeq ($(MESA_LLVM),1)
common_SYS += $(LLVM_LIBS)
egl_LIBS += $(TOP)/src/gallium/drivers/llvmpipe/libllvmpipe.a
LDFLAGS += $(LLVM_LDFLAGS)
endif

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
	$(TOP)/src/gallium/drivers/i965/libi965.a

# nouveau pipe driver
nouveau_CPPFLAGS :=
nouveau_SYS := -ldrm_nouveau
nouveau_LIBS := \
	$(TOP)/src/gallium/winsys/nouveau/drm/libnouveaudrm.a \
	$(TOP)/src/gallium/drivers/nvfx/libnvfx.a \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a \
	$(TOP)/src/gallium/drivers/nouveau/libnouveau.a

# radeon pipe driver
radeon_CPPFLAGS :=
radeon_SYS := -ldrm -ldrm_radeon
radeon_LIBS := \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/r300/libr300.a

# vmwgfx pipe driver
vmwgfx_CPPFLAGS :=
vmwgfx_SYS :=
vmwgfx_LIBS := \
	$(TOP)/src/gallium/winsys/svga/drm/libsvgadrm.a \
	$(TOP)/src/gallium/drivers/svga/libsvga.a

# swrast (pseudo) pipe driver
swrast_CPPFLAGS :=
swrast_SYS :=
swrast_LIBS :=

# OpenGL state tracker
GL_CPPFLAGS := -I$(TOP)/src/mesa $(API_DEFINES)
GL_SYS := -lpthread -lm -L$(TOP)/$(LIB_DIR) -l$(GL_LIB)
GL_LIBS := $(TOP)/src/mesa/libmesagallium.a

# OpenGL ES 1.x state tracker
GLESv1_CM_CPPFLAGS := -I$(TOP)/src/mesa
GLESv1_CM_SYS := -lpthread -lm -L$(TOP)/$(LIB_DIR) -l$(GLESv1_CM_LIB)
GLESv1_CM_LIBS := $(TOP)/src/mesa/libes1gallium.a

# OpenGL ES 2.x state tracker
GLESv2_CPPFLAGS := -I$(TOP)/src/mesa
GLESv2_SYS := -lpthread -lm -L$(TOP)/$(LIB_DIR) -l$(GLESv2_LIB)
GLESv2_LIBS := $(TOP)/src/mesa/libes2gallium.a

# OpenVG state tracker
OpenVG_CPPFLAGS := -I$(TOP)/src/gallium/state_trackers/vega
OpenVG_SYS := -lm -L$(TOP)/$(LIB_DIR) -l$(VG_LIB)
OpenVG_LIBS := $(TOP)/src/gallium/state_trackers/vega/libvega.a


OUTPUT_PATH := $(TOP)/$(LIB_DIR)/egl

# determine the outputs
ifneq ($(findstring i915,$(GALLIUM_WINSYS_DIRS)),)
OUTPUTS += i915
endif
ifneq ($(findstring i965,$(GALLIUM_WINSYS_DIRS)),)
OUTPUTS += i965
endif
ifneq ($(findstring nouveau,$(GALLIUM_WINSYS_DIRS)),)
OUTPUTS += nouveau
endif
ifneq ($(findstring r300,$(GALLIUM_WINSYS_DIRS)),)
OUTPUTS += radeon
endif
ifneq ($(findstring svga,$(GALLIUM_WINSYS_DIRS)),)
OUTPUTS += vmwgfx
endif
OUTPUTS += swrast
OUTPUTS := $(addprefix egl_gallium_, $(OUTPUTS))

# state trackers
OUTPUTS += $(addprefix st_, $(EGL_CLIENT_APIS))

OUTPUTS := $(addsuffix .so, $(OUTPUTS))
OUTPUTS := $(addprefix $(OUTPUT_PATH)/, $(OUTPUTS))

default: $(OUTPUTS)

define mklib-egl
$(MKLIB) -o $(notdir $@) -noprefix -linker '$(CC)' -ldflags '$(LDFLAGS)' \
	-install $(OUTPUT_PATH) $(MKLIB_OPTIONS) $< \
	-Wl,--start-group $(common_LIBS) $(egl_LIBS) $($(1)_LIBS) -Wl,--end-group \
	$(common_SYS) $(egl_SYS) $($(1)_SYS)
endef

define mklib
$(MKLIB) -o $(notdir $@) -noprefix -linker '$(CC)' -ldflags '$(LDFLAGS)' \
	-install $(OUTPUT_PATH) $(MKLIB_OPTIONS) $< \
	-Wl,--start-group $(common_LIBS) $($(1)_LIBS) -Wl,--end-group \
	$(common_SYS) $($(1)_SYS)
endef

# EGL drivers
$(OUTPUT_PATH)/egl_gallium_i915.so: pipe_i915.o $(egl_LIBS) $(i915_LIBS)
	$(call mklib-egl,i915)

$(OUTPUT_PATH)/egl_gallium_i965.so: pipe_i965.o $(egl_LIBS) $(i965_LIBS)
	$(call mklib-egl,i965)

$(OUTPUT_PATH)/egl_gallium_nouveau.so: pipe_nouveau.o $(egl_LIBS) $(nouveau_LIBS)
	$(call mklib-egl,nouveau)

$(OUTPUT_PATH)/egl_gallium_radeon.so: pipe_radeon.o $(egl_LIBS) $(radeon_LIBS)
	$(call mklib-egl,radeon)

$(OUTPUT_PATH)/egl_gallium_vmwgfx.so: pipe_vmwgfx.o $(egl_LIBS) $(vmwgfx_LIBS)
	$(call mklib-egl,vmwgfx)

$(OUTPUT_PATH)/egl_gallium_swrast.so: pipe_swrast.o $(egl_LIBS) $(swrast_LIBS)
	$(call mklib-egl,swrast)

# state trackers
$(OUTPUT_PATH)/st_$(GL_LIB).so: st_GL.o $(GL_LIBS)
	$(call mklib,GL)

$(OUTPUT_PATH)/st_$(GLESv1_CM_LIB).so: st_GLESv1_CM.o $(GLESv1_CM_LIBS)
	$(call mklib,GLESv1_CM)

$(OUTPUT_PATH)/st_$(GLESv2_LIB).so: st_GLESv2.o $(GLESv2_LIBS)
	$(call mklib,GLESv2)

$(OUTPUT_PATH)/st_$(VG_LIB).so: st_OpenVG.o $(OpenVG_LIBS)
	$(call mklib,OpenVG)

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
