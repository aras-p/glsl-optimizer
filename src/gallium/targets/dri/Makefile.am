include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	$(GALLIUM_TARGET_CFLAGS)

AM_CPPFLAGS = \
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
	-Wl,--dynamic-list=$(top_srcdir)/src/gallium/targets/dri-vdpau.dyn \
	$(GC_SECTIONS)

if HAVE_LD_VERSION_SCRIPT
gallium_dri_la_LDFLAGS += \
	-Wl,--version-script=$(top_srcdir)/src/gallium/targets/dri/dri.sym
endif # HAVE_LD_VERSION_SCRIPT

# XXX: Fold libdridrm/libdrisw as drivers become megadriver (__dri*DriverExtension) aware
gallium_dri_la_LIBADD = \
	$(top_builddir)/src/mesa/libmesagallium.la \
	$(top_builddir)/src/mesa/drivers/dri/common/libdricommon.la \
	$(top_builddir)/src/gallium/state_trackers/dri/drm/libdridrm_s.la \
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

MEGADRIVERS =
STATIC_TARGET_CPPFLAGS =
STATIC_TARGET_LIB_DEPS = \
	$(top_builddir)/src/loader/libloader.la

if HAVE_GALLIUM_NOUVEAU
MEGADRIVERS += nouveau
STATIC_TARGET_CPPFLAGS += -DGALLIUM_NOUVEAU
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/nouveau/drm/libnouveaudrm.la \
	$(top_builddir)/src/gallium/drivers/nouveau/libnouveau.la \
	$(NOUVEAU_LIBS)
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
