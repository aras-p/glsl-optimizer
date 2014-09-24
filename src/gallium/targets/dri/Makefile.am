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

# XXX: Temporary allow duplicated symbols, as the loader pulls in xmlconfig.c
# which already provides driParse* and driQuery* amongst others.
# Remove this hack as we come up with a cleaner solution.
gallium_dri_la_LDFLAGS += \
	-Wl,--allow-multiple-definition

TARGET_DRIVERS =
TARGET_CPPFLAGS =
TARGET_LIB_DEPS = $(top_builddir)/src/loader/libloader.la

include $(top_srcdir)/src/gallium/drivers/i915/Automake.inc

include $(top_srcdir)/src/gallium/drivers/ilo/Automake.inc

include $(top_srcdir)/src/gallium/drivers/nouveau/Automake.inc

include $(top_srcdir)/src/gallium/drivers/r300/Automake.inc
include $(top_srcdir)/src/gallium/drivers/r600/Automake.inc
include $(top_srcdir)/src/gallium/drivers/radeonsi/Automake.inc

include $(top_srcdir)/src/gallium/drivers/svga/Automake.inc

include $(top_srcdir)/src/gallium/drivers/freedreno/Automake.inc

include $(top_srcdir)/src/gallium/drivers/vc4/Automake.inc

include $(top_srcdir)/src/gallium/drivers/softpipe/Automake.inc
include $(top_srcdir)/src/gallium/drivers/llvmpipe/Automake.inc

if HAVE_GALLIUM_STATIC_TARGETS

gallium_dri_la_SOURCES += target.c
gallium_dri_la_CPPFLAGS = $(AM_CPPFLAGS) $(TARGET_CPPFLAGS)
gallium_dri_la_LIBADD += $(TARGET_LIB_DEPS) \
	$(TARGET_RADEON_WINSYS) $(TARGET_RADEON_COMMON)

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
	for i in $(TARGET_DRIVERS); do                                  \
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
	for i in $(TARGET_DRIVERS); do                                  \
		j=gallium_dri.so;                                       \
		k=$${i}_dri.so;                                         \
		ln -f $${dest_dir}/$${j}                                \
			$${dest_dir}/$${k};                             \
	done;                                                           \
	$(RM) -f $$dest_dir/gallium_dri.*
