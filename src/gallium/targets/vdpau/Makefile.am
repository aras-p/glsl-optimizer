include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	$(GALLIUM_TARGET_CFLAGS)

vdpaudir = $(VDPAU_LIB_INSTALL_DIR)
vdpau_LTLIBRARIES = libvdpau_gallium.la

nodist_EXTRA_libvdpau_gallium_la_SOURCES = dummy.cpp
libvdpau_gallium_la_SOURCES = \
	$(top_srcdir)/src/gallium/auxiliary/vl/vl_winsys_dri.c

libvdpau_gallium_la_LDFLAGS = \
	-shared \
	-module \
	-no-undefined \
	-version-number $(VDPAU_MAJOR):$(VDPAU_MINOR) \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

if HAVE_LD_VERSION_SCRIPT
libvdpau_gallium_la_LDFLAGS += \
	-Wl,--version-script=$(top_srcdir)/src/gallium/targets/vdpau/vdpau.sym
endif # HAVE_LD_VERSION_SCRIPT

if HAVE_LD_DYNAMIC_LIST
libvdpau_gallium_la_LDFLAGS += \
	-Wl,--dynamic-list=$(top_srcdir)/src/gallium/targets/dri-vdpau.dyn
endif # HAVE_LD_DYNAMIC_LIST

libvdpau_gallium_la_LIBADD = \
	$(top_builddir)/src/gallium/state_trackers/vdpau/libvdpautracker.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/util/libmesautil.la \
	$(VDPAU_LIBS) \
	$(LIBDRM_LIBS) \
	$(GALLIUM_COMMON_LIB_DEPS)


if HAVE_GALLIUM_STATIC_TARGETS

MEGADRIVERS =
STATIC_TARGET_CPPFLAGS = -DGALLIUM_STATIC_TARGETS=1
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

# Radeon winsys chaos
if HAVE_GALLIUM_R600
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/radeon/drm/libradeonwinsys.la
else
if HAVE_GALLIUM_RADEONSI
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/radeon/drm/libradeonwinsys.la
endif
endif

if HAVE_GALLIUM_RADEON_COMMON
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/drivers/radeon/libradeon.la
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

libvdpau_gallium_la_SOURCES += target.c
libvdpau_gallium_la_CPPFLAGS = $(STATIC_TARGET_CPPFLAGS)
libvdpau_gallium_la_LIBADD += $(STATIC_TARGET_LIB_DEPS)

else # HAVE_GALLIUM_STATIC_TARGETS

libvdpau_gallium_la_CPPFLAGS = \
	$(GALLIUM_PIPE_LOADER_DEFINES) \
	-DPIPE_SEARCH_DIR=\"$(libdir)/gallium-pipe\"

# XXX: Use the pipe-loader-client over pipe-loader ?
libvdpau_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/auxiliary/pipe-loader/libpipe_loader.la \
	$(GALLIUM_PIPE_LOADER_WINSYS_LIBS) \
	$(GALLIUM_PIPE_LOADER_LIBS)

endif # HAVE_GALLIUM_STATIC_TARGETS

if HAVE_MESA_LLVM
libvdpau_gallium_la_LIBADD += $(LLVM_LIBS)
libvdpau_gallium_la_LDFLAGS += $(LLVM_LDFLAGS)
endif

if HAVE_COMPAT_SYMLINKS
# Add a link to allow setting VDPAU_DRIVER_PATH to /lib/gallium of the build tree.
all-local: $(vdpau_LTLIBRARIES)
	$(AM_V_GEN)link_dir=$(top_builddir)/$(LIB_DIR)/gallium;		\
	$(MKDIR_P) $${link_dir};					\
	for i in $(MEGADRIVERS); do					\
		j=libvdpau_gallium.$(LIB_EXT);				\
		k=libvdpau_$${i}.$(LIB_EXT);				\
		l=$${k}.$(VDPAU_MAJOR).$(VDPAU_MINOR).0;		\
		ln -f .libs/$${j}.$(VDPAU_MAJOR).$(VDPAU_MINOR).0	\
		      $${link_dir}/$${l};				\
		ln -sf $${l}						\
		       $${link_dir}/$${k}.$(VDPAU_MAJOR).$(VDPAU_MINOR); \
		ln -sf $${l}						\
		       $${link_dir}/$${k}.$(VDPAU_MAJOR);		\
		ln -sf $${l}						\
		       $${link_dir}/$${k};				\
	done
endif

# hardlink each megadriver instance, but don't actually have
# libvdpau_gallium.so in the set of final installed files.
install-data-hook:
	$(AM_V_GEN)dest_dir=$(DESTDIR)/$(vdpaudir);			\
	for i in $(MEGADRIVERS); do					\
		j=libvdpau_gallium.$(LIB_EXT);				\
		k=libvdpau_$${i}.$(LIB_EXT);				\
		l=$${k}.$(VDPAU_MAJOR).$(VDPAU_MINOR).0;		\
		ln -f $${dest_dir}/$${j}.$(VDPAU_MAJOR).$(VDPAU_MINOR).0 \
		      $${dest_dir}/$${l};				\
		ln -sf $${l}						\
		       $${dest_dir}/$${k}.$(VDPAU_MAJOR).$(VDPAU_MINOR); \
		ln -sf $${l}						\
		       $${dest_dir}/$${k}.$(VDPAU_MAJOR);		\
		ln -sf $${l}						\
		       $${dest_dir}/$${k};				\
	done;								\
	$(RM) -f $${dest_dir}/libvdpau_gallium.*
