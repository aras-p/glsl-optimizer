include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	$(GALLIUM_TARGET_CFLAGS)

xvmcdir = $(XVMC_LIB_INSTALL_DIR)
xvmc_LTLIBRARIES = libXvMCgallium.la

nodist_EXTRA_libXvMCgallium_la_SOURCES = dummy.cpp
libXvMCgallium_la_SOURCES = \
	$(top_srcdir)/src/gallium/auxiliary/vl/vl_winsys_dri.c

libXvMCgallium_la_LDFLAGS = \
	-shared \
	-module \
	-no-undefined \
	-version-number $(XVMC_MAJOR):$(XVMC_MINOR) \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

if HAVE_LD_VERSION_SCRIPT
libXvMCgallium_la_LDFLAGS += \
	-Wl,--version-script=$(top_srcdir)/src/gallium/targets/xvmc/xvmc.sym
endif # HAVE_LD_VERSION_SCRIPT

libXvMCgallium_la_LIBADD = \
	$(top_builddir)/src/gallium/state_trackers/xvmc/libxvmctracker.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/util/libmesautil.la \
	$(XVMC_LIBS) \
	$(LIBDRM_LIBS) \
	$(GALLIUM_COMMON_LIB_DEPS)


TARGET_DRIVERS =
TARGET_CPPFLAGS =
TARGET_LIB_DEPS = $(top_builddir)/src/loader/libloader.la

include $(top_srcdir)/src/gallium/drivers/nouveau/Automake.inc

include $(top_srcdir)/src/gallium/drivers/r600/Automake.inc

if HAVE_GALLIUM_STATIC_TARGETS

libXvMCgallium_la_SOURCES += target.c
libXvMCgallium_la_CPPFLAGS = $(TARGET_CPPFLAGS) -DGALLIUM_STATIC_TARGETS=1
libXvMCgallium_la_LIBADD += $(TARGET_LIB_DEPS) \
	$(TARGET_RADEON_WINSYS) $(TARGET_RADEON_COMMON)

else # HAVE_GALLIUM_STATIC_TARGETS

libXvMCgallium_la_CPPFLAGS = \
	$(GALLIUM_PIPE_LOADER_DEFINES) \
	-DPIPE_SEARCH_DIR=\"$(libdir)/gallium-pipe\"

# XXX: Use the pipe-loader-client over pipe-loader ?
libXvMCgallium_la_LIBADD += \
	$(top_builddir)/src/gallium/auxiliary/pipe-loader/libpipe_loader.la \
	$(GALLIUM_PIPE_LOADER_WINSYS_LIBS) \
	$(GALLIUM_PIPE_LOADER_LIBS)

endif # HAVE_GALLIUM_STATIC_TARGETS

if HAVE_MESA_LLVM
libXvMCgallium_la_LIBADD += $(LLVM_LIBS)
libXvMCgallium_la_LDFLAGS += $(LLVM_LDFLAGS)
endif

# hardlink each megadriver instance, but don't actually have
# libXvMCgallium.so in the set of final installed files.
install-data-hook:
	$(AM_V_GEN)dest_dir=$(DESTDIR)/$(xvmcdir);			\
	for i in $(TARGET_DRIVERS); do					\
		j=libXvMCgallium.$(LIB_EXT);				\
		k=libXvMC$${i}.$(LIB_EXT);				\
		l=$${k}.$(XVMC_MAJOR).$(XVMC_MINOR).0;			\
		ln -f $${dest_dir}/$${j}.$(XVMC_MAJOR).$(XVMC_MINOR).0	\
		      $${dest_dir}/$${l};				\
		ln -sf $${l}						\
		       $${dest_dir}/$${k}.$(XVMC_MAJOR).$(XVMC_MINOR);	\
		ln -sf $${l}						\
		       $${dest_dir}/$${k}.$(XVMC_MAJOR);		\
		ln -sf $${l}						\
		       $${dest_dir}/$${k};				\
	done;								\
	$(RM) -f $${dest_dir}/libXvMCgallium.*
