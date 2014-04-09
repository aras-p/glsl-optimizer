dridir = $(DRI_DRIVER_INSTALL_DIR)

SUBDIRS =
MEGADRIVERS =
MEGADRIVERS_DEPS =

SUBDIRS+=common

if HAVE_I915_DRI
SUBDIRS += i915
MEGADRIVERS_DEPS += i915/libi915_dri.la
MEGADRIVERS += i915_dri.so
endif

if HAVE_I965_DRI
SUBDIRS += i965
MEGADRIVERS_DEPS += i965/libi965_dri.la
MEGADRIVERS += i965_dri.so
endif

if HAVE_NOUVEAU_DRI
SUBDIRS += nouveau
MEGADRIVERS_DEPS += nouveau/libnouveau_dri.la
MEGADRIVERS += nouveau_vieux_dri.so
endif

if HAVE_R200_DRI
SUBDIRS += r200
MEGADRIVERS_DEPS += r200/libr200_dri.la
MEGADRIVERS += r200_dri.so
endif

if HAVE_RADEON_DRI
SUBDIRS += radeon
MEGADRIVERS_DEPS += radeon/libradeon_dri.la
MEGADRIVERS += radeon_dri.so
endif

if HAVE_SWRAST_DRI
SUBDIRS += swrast
MEGADRIVERS_DEPS += swrast/libswrast_dri.la
MEGADRIVERS += swrast_dri.so
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dri.pc

driincludedir = $(includedir)/GL/internal
driinclude_HEADERS = $(top_srcdir)/include/GL/internal/dri_interface.h

nodist_EXTRA_mesa_dri_drivers_la_SOURCES = dummy.cpp
mesa_dri_drivers_la_SOURCES =
mesa_dri_drivers_la_LDFLAGS = \
        -module -avoid-version -shared -shrext .so \
        -Wl,-Bsymbolic \
        $(GC_SECTIONS) \
        $()
mesa_dri_drivers_la_LIBADD = \
        ../../libmesa.la \
        common/libmegadriver_stub.la \
        common/libdricommon.la \
        $(MEGADRIVERS_DEPS) \
        $(DRI_LIB_DEPS) \
        $()

if NEED_MEGADRIVER
dri_LTLIBRARIES = mesa_dri_drivers.la

if HAVE_COMPAT_SYMLINKS
# Add a link to allow setting LD_LIBRARY_PATH/LIBGL_DRIVERS_PATH to /lib of the build tree.
all-local: mesa_dri_drivers.la
	$(AM_V_at)$(MKDIR_P) $(top_builddir)/$(LIB_DIR);
	$(AM_V_GEN)ln -f .libs/mesa_dri_drivers.so \
			 $(top_builddir)/$(LIB_DIR)/mesa_dri_drivers.so;
	$(AM_V_GEN)for i in $(MEGADRIVERS); do \
		ln -f $(top_builddir)/$(LIB_DIR)/mesa_dri_drivers.so \
		      $(top_builddir)/$(LIB_DIR)/$$i; \
	done;
endif

# hardlink each megadriver instance, but don't actually have
# mesa_dri_drivers.so in the set of final installed files.
install-data-hook:
	for i in $(MEGADRIVERS); do \
		ln -f $(DESTDIR)$(dridir)/mesa_dri_drivers.so \
		      $(DESTDIR)$(dridir)/$$i; \
	done;
	$(RM) -f $(DESTDIR)$(dridir)/mesa_dri_drivers.so
	$(RM) -f $(DESTDIR)$(dridir)/mesa_dri_drivers.la

endif
