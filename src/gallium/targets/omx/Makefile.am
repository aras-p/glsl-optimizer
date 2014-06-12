include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	$(GALLIUM_VIDEO_CFLAGS)

omxdir = $(OMX_LIB_INSTALL_DIR)
omx_LTLIBRARIES = libomx_mesa.la

nodist_EXTRA_libomx_mesa_la_SOURCES = dummy.cpp
libomx_mesa_la_SOURCES = \
	$(top_srcdir)/src/gallium/auxiliary/vl/vl_winsys_dri.c

libomx_mesa_la_LDFLAGS = \
	-shared \
	-module \
	-no-undefined \
	-avoid-version \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

if HAVE_LD_VERSION_SCRIPT
libomx_mesa_la_LDFLAGS += \
	-Wl,--version-script=$(top_srcdir)/src/gallium/targets/omx/omx.sym
endif # HAVE_LD_VERSION_SCRIPT

libomx_mesa_la_LIBADD = \
	$(top_builddir)/src/gallium/state_trackers/omx/libomxtracker.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(OMX_LIBS) \
	$(GALLIUM_COMMON_LIB_DEPS)

if HAVE_GALLIUM_STATIC_TARGETS

STATIC_TARGET_CPPFLAGS = -DGALLIUM_STATIC_TARGETS=1
STATIC_TARGET_LIB_DEPS = \
	$(top_builddir)/src/loader/libloader.la

if HAVE_GALLIUM_NOUVEAU
STATIC_TARGET_CPPFLAGS += -DGALLIUM_NOUVEAU
STATIC_TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/nouveau/drm/libnouveaudrm.la \
	$(top_builddir)/src/gallium/drivers/nouveau/libnouveau.la \
	$(NOUVEAU_LIBS)
endif

libomx_mesa_la_SOURCES += target.c
libomx_mesa_la_CPPFLAGS = $(STATIC_TARGET_CPPFLAGS)
libomx_mesa_la_LIBADD += $(STATIC_TARGET_LIB_DEPS)

else # HAVE_GALLIUM_STATIC_TARGETS

libomx_mesa_la_CPPFLAGS = \
	$(GALLIUM_PIPE_LOADER_DEFINES) \
	-DPIPE_SEARCH_DIR=\"$(libdir)/gallium-pipe\"

# XXX: Use the pipe-loader-client over pipe-loader ?
libomx_mesa_la_LIBADD += \
	$(top_builddir)/src/gallium/auxiliary/pipe-loader/libpipe_loader.la \
	$(GALLIUM_PIPE_LOADER_WINSYS_LIBS) \
	$(GALLIUM_PIPE_LOADER_LIBS)

endif # HAVE_GALLIUM_STATIC_TARGETS

if HAVE_MESA_LLVM
libomx_mesa_la_LIBADD += $(LLVM_LIBS)
libomx_mesa_la_LDFLAGS += $(LLVM_LDFLAGS)
endif
