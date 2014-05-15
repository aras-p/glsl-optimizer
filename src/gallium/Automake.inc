GALLIUM_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/auxiliary \
	$(DEFINES)

# src/gallium/auxiliary must appear before src/gallium/drivers
# because there are stupidly two rbug_context.h files in
# different directories, and which one is included by the
# preprocessor is determined by the ordering of the -I flags.
GALLIUM_DRIVER_CFLAGS = \
	-I$(srcdir)/include \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/auxiliary \
	-I$(top_srcdir)/src/gallium/drivers \
	$(DEFINES) \
	$(VISIBILITY_CFLAGS)

GALLIUM_DRIVER_CXXFLAGS = \
	-I$(srcdir)/include \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/auxiliary \
	-I$(top_srcdir)/src/gallium/drivers \
	$(DEFINES) \
	$(VISIBILITY_CXXFLAGS)

GALLIUM_DRI_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/auxiliary \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/gallium/winsys \
	-I$(top_srcdir)/src/mesa \
	-I$(top_srcdir)/src/mapi \
	$(DEFINES) \
	$(PTHREAD_CFLAGS) \
	$(LIBDRM_CFLAGS) \
	$(VISIBILITY_CFLAGS)

GALLIUM_VIDEO_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/auxiliary \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/gallium/winsys \
	$(DEFINES) \
	$(PTHREAD_CFLAGS) \
	$(LIBDRM_CFLAGS) \
	$(VISIBILITY_CFLAGS)


GALLIUM_DRI_LINKER_FLAGS = \
	-shared \
	-shrext .so \
	-module \
	-avoid-version \
	$(GC_SECTIONS)

GALLIUM_VDPAU_LINKER_FLAGS = \
	-shared \
	-module \
	-no-undefined \
	-version-number $(VDPAU_MAJOR):$(VDPAU_MINOR) \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

GALLIUM_XVMC_LINKER_FLAGS = \
	-shared \
	-module \
	-no-undefined \
	-version-number $(XVMC_MAJOR):$(XVMC_MINOR) \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

GALLIUM_OMX_LINKER_FLAGS = \
	-shared \
	-module \
	-no-undefined \
	-avoid-version \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

if HAVE_LD_VERSION_SCRIPT
GALLIUM_DRI_LINKER_FLAGS += \
	-Wl,--version-script=$(top_srcdir)/src/gallium/targets/dri.sym

GALLIUM_VDPAU_LINKER_FLAGS += \
	-Wl,--version-script=$(top_srcdir)/src/gallium/targets/vdpau.sym

GALLIUM_XVMC_LINKER_FLAGS += \
	-Wl,--version-script=$(top_srcdir)/src/gallium/targets/xvmc.sym

GALLIUM_OMX_LINKER_FLAGS += \
	-Wl,--version-script=$(top_srcdir)/src/gallium/targets/omx.sym
endif


GALLIUM_COMMON_LIB_DEPS = \
	-lm \
	$(CLOCK_LIB) \
	$(PTHREAD_LIBS) \
	$(DLOPEN_LIBS)

GALLIUM_DRI_LIB_DEPS = \
	$(top_builddir)/src/mesa/libmesagallium.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(SELINUX_LIBS) \
	$(LIBDRM_LIBS) \
	$(EXPAT_LIBS) \
	$(GALLIUM_COMMON_LIB_DEPS)

GALLIUM_VDPAU_LIB_DEPS = \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/gallium/state_trackers/vdpau/libvdpautracker.la \
	$(VDPAU_LIBS) \
	$(LIBDRM_LIBS) \
	$(GALLIUM_COMMON_LIB_DEPS)

GALLIUM_XVMC_LIB_DEPS = \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/gallium/state_trackers/xvmc/libxvmctracker.la \
	$(XVMC_LIBS) \
	$(LIBDRM_LIBS) \
	$(GALLIUM_COMMON_LIB_DEPS)

GALLIUM_OMX_LIB_DEPS = \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/gallium/state_trackers/omx/libomxtracker.la \
	$(OMX_LIBS) \
	$(GALLIUM_COMMON_LIB_DEPS)

GALLIUM_WINSYS_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/auxiliary \
	$(DEFINES) \
	$(VISIBILITY_CFLAGS)

if HAVE_MESA_LLVM

GALLIUM_DRI_LINKER_FLAGS += $(LLVM_LDFLAGS)
GALLIUM_VDPAU_LINKER_FLAGS += $(LLVM_LDFLAGS)
GALLIUM_XVMC_LINKER_FLAGS += $(LLVM_LDFLAGS)
GALLIUM_OMX_LINKER_FLAGS += $(LLVM_LDFLAGS)

GALLIUM_DRI_LIB_DEPS += $(LLVM_LIBS)
GALLIUM_VDPAU_LIB_DEPS += $(LLVM_LIBS)
GALLIUM_XVMC_LIB_DEPS += $(LLVM_LIBS)
GALLIUM_OMX_LIB_DEPS += $(LLVM_LIBS)

endif


GALLIUM_PIPE_LOADER_WINSYS_LIBS = \
	$(top_builddir)/src/gallium/winsys/sw/null/libws_null.la

if HAVE_DRISW
GALLIUM_PIPE_LOADER_WINSYS_LIBS += \
	$(top_builddir)/src/gallium/winsys/sw/dri/libswdri.la
endif

if NEED_WINSYS_XLIB
GALLIUM_PIPE_LOADER_WINSYS_LIBS += \
	$(top_builddir)/src/gallium/winsys/sw/xlib/libws_xlib.la \
	-lX11 -lXext -lXfixes \
	$(LIBDRM_LIBS)
endif
