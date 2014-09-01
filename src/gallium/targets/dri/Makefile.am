include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/src/mesa \
	-I$(top_srcdir)/src/mesa/drivers/dri/common \
	-I$(top_srcdir)/src/gallium/state_trackers/dri \
	$(GALLIUM_TARGET_CFLAGS)

AM_CPPFLAGS = \
	$(DEFINES) \
	-DDRI_TARGET \
	-DGALLIUM_GALAHAD \
	-DGALLIUM_NOOP \
	-DGALLIUM_RBUG \
	-DGALLIUM_TRACE

dridir = $(DRI_DRIVER_INSTALL_DIR)
dri_LTLIBRARIES = gallium_dri.la

nodist_EXTRA_gallium_dri_la_SOURCES = dummy.cpp
gallium_dri_la_SOURCES =

gallium_dri_la_LDFLAGS = \
	-shared \
	-shrext .so \
	-module \
	-avoid-version \
	$(GC_SECTIONS)

if HAVE_LD_VERSION_SCRIPT
gallium_dri_la_LDFLAGS += \
	-Wl,--version-script=$(top_srcdir)/src/gallium/targets/dri/dri.sym
endif # HAVE_LD_VERSION_SCRIPT

if HAVE_LD_DYNAMIC_LIST
gallium_dri_la_LDFLAGS += \
	-Wl,--dynamic-list=$(top_srcdir)/src/gallium/targets/dri-vdpau.dyn
endif # HAVE_LD_DYNAMIC_LIST

gallium_dri_la_LIBADD = \
	$(top_builddir)/src/mesa/libmesagallium.la \
	$(top_builddir)/src/mesa/drivers/dri/common/libdricommon.la \
	$(top_builddir)/src/mesa/drivers/dri/common/libmegadriver_stub.la \
	$(top_builddir)/src/gallium/state_trackers/dri/libdri.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/gallium/drivers/galahad/libgalahad.la \
	$(top_builddir)/src/gallium/drivers/noop/libnoop.la \
	$(top_builddir)/src/gallium/drivers/rbug/librbug.la \
	$(top_builddir)/src/gallium/drivers/trace/libtrace.la \
	$(SELINUX_LIBS) \
	$(EXPAT_LIBS) \
	$(LIBDRM_LIBS) \
	$(GALLIUM_COMMON_LIB_DEPS)

if HAVE_GALLIUM_STATIC_TARGETS

# XXX: Temporary allow duplicated symbols, as the loader pulls in xmlconfig.c
# which already provides driParse* and driQuery* amongst others.
# Remove this hack as we come up with a cleaner solution.
gallium_dri_la_LDFLAGS += \
	-Wl,--allow-multiple-definition

MEGADRIVERS =
STATIC_TARGET_CPPFLAGS =
STATIC_TARGET_LIB_DEPS = \
	$(top_builddir)/src/loader/libloader.la

if HAVE_GALLIUM_I915
MEGADRIVERS += i915
STATIC_TARGET_CPPFLAGS += -DGALLIUM_I915
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/i915/drm/libi915drm.la \
	$(top_builddir)/src/gallium/drivers/i915/libi915.la \
	$(INTEL_LIBS)
endif

if HAVE_GALLIUM_ILO
MEGADRIVERS += ilo
STATIC_TARGET_CPPFLAGS += -DGALLIUM_ILO
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/intel/drm/libintelwinsys.la \
	$(top_builddir)/src/gallium/drivers/ilo/libilo.la \
	$(INTEL_LIBS)
endif

if HAVE_GALLIUM_NOUVEAU
MEGADRIVERS += nouveau
STATIC_TARGET_CPPFLAGS += -DGALLIUM_NOUVEAU
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/nouveau/drm/libnouveaudrm.la \
	$(top_builddir)/src/gallium/drivers/nouveau/libnouveau.la \
	$(NOUVEAU_LIBS)
endif

if NEED_RADEON_DRM_WINSYS
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/radeon/drm/libradeonwinsys.la
endif

if HAVE_GALLIUM_RADEON_COMMON
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/drivers/radeon/libradeon.la
endif

if HAVE_GALLIUM_R300
MEGADRIVERS += r300
STATIC_TARGET_CPPFLAGS += -DGALLIUM_R300
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/drivers/r300/libr300.la \
	$(RADEON_LIBS)
endif

if HAVE_GALLIUM_R600
MEGADRIVERS += r600
STATIC_TARGET_CPPFLAGS += -DGALLIUM_R600
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/drivers/r600/libr600.la \
	$(RADEON_LIBS)
endif

if HAVE_GALLIUM_RADEONSI
MEGADRIVERS += radeonsi
STATIC_TARGET_CPPFLAGS += -DGALLIUM_RADEONSI
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/drivers/radeonsi/libradeonsi.la \
	$(RADEON_LIBS)
endif

if HAVE_GALLIUM_SVGA
MEGADRIVERS += vmwgfx
STATIC_TARGET_CPPFLAGS += -DGALLIUM_VMWGFX
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/svga/drm/libsvgadrm.la \
	$(top_builddir)/src/gallium/drivers/svga/libsvga.la
endif

if HAVE_GALLIUM_FREEDRENO
MEGADRIVERS += msm kgsl
STATIC_TARGET_CPPFLAGS += -DGALLIUM_FREEDRENO
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/freedreno/drm/libfreedrenodrm.la \
	$(top_builddir)/src/gallium/drivers/freedreno/libfreedreno.la \
	$(FREEDRENO_LIBS)

endif

if HAVE_GALLIUM_VC4
MEGADRIVERS += vc4
STATIC_TARGET_CPPFLAGS += -DGALLIUM_VC4
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/vc4/drm/libvc4drm.la \
	$(top_builddir)/src/gallium/drivers/vc4/libvc4.la

if USE_VC4_SIMULATOR
STATIC_TARGET_CPPFLAGS += -DUSE_VC4_SIMULATOR
endif
endif

if HAVE_GALLIUM_SOFTPIPE
MEGADRIVERS += swrast
STATIC_TARGET_CPPFLAGS += -DGALLIUM_SOFTPIPE
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/sw/dri/libswdri.la \
	$(top_builddir)/src/gallium/drivers/softpipe/libsoftpipe.la

if HAVE_DRI2
MEGADRIVERS += kms_swrast
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/sw/kms-dri/libswkmsdri.la
endif
endif

if NEED_GALLIUM_LLVMPIPE_DRIVER
STATIC_TARGET_CPPFLAGS += -DGALLIUM_LLVMPIPE
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/drivers/llvmpipe/libllvmpipe.la
endif

gallium_dri_la_SOURCES += target.c
AM_CPPFLAGS += $(STATIC_TARGET_CPPFLAGS)
gallium_dri_la_LIBADD += $(STATIC_TARGET_LIB_DEPS)

else # HAVE_GALLIUM_STATIC_TARGETS

gallium_dri_la_LIBADD += \
	$(top_builddir)/src/gallium/auxiliary/pipe-loader/libpipe_loader.la \
	$(GALLIUM_PIPE_LOADER_WINSYS_LIBS) \
	$(GALLIUM_PIPE_LOADER_LIBS)

endif # HAVE_GALLIUM_STATIC_TARGETS

if HAVE_MESA_LLVM
gallium_dri_la_LIBADD += $(LLVM_LIBS)
gallium_dri_la_LDFLAGS += $(LLVM_LDFLAGS)
endif

if HAVE_COMPAT_SYMLINKS
# Add a link to allow setting LIBGL_DRIVERS_PATH to /lib/gallium of the build tree.
all-local: $(dri_LTLIBRARIES)
	$(AM_V_GEN)link_dir=$(top_builddir)/$(LIB_DIR)/gallium;         \
	$(MKDIR_P) $${link_dir};                                        \
	for i in $(MEGADRIVERS); do                                     \
		j=gallium_dri.so;                                       \
		k=$${i}_dri.so;                                         \
		ln -f .libs/$${j}                                       \
			$${link_dir}/$${k};                             \
	done
endif

# hardlink each megadriver instance, but don't actually have
# gallium_dri.so in the set of final installed files.
install-data-hook:
	$(AM_V_GEN)dest_dir=$(DESTDIR)/$(dridir);                       \
	for i in $(MEGADRIVERS); do                                     \
		j=gallium_dri.so;                                       \
		k=$${i}_dri.so;                                         \
		ln -f $${dest_dir}/$${j}                                \
			$${dest_dir}/$${k};                             \
	done;                                                           \
	$(RM) -f $$dest_dir/gallium_dri.*
