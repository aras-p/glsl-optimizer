include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	$(GALLIUM_TARGET_CFLAGS)

vadir = $(VA_LIB_INSTALL_DIR)
va_LTLIBRARIES = gallium_drv_video.la

nodist_EXTRA_gallium_drv_video_la_SOURCES = dummy.cpp
gallium_drv_video_la_SOURCES = \
	$(top_srcdir)/src/gallium/auxiliary/vl/vl_winsys_dri.c

gallium_drv_video_la_LDFLAGS = \
	-shared \
	-module \
	-no-undefined \
	-avoid-version \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

if HAVE_LD_VERSION_SCRIPT
gallium_drv_video_la_LDFLAGS += \
	-Wl,--version-script=$(top_srcdir)/src/gallium/targets/va/va.sym
endif # HAVE_LD_VERSION_SCRIPT

gallium_drv_video_la_LIBADD = \
	$(top_builddir)/src/gallium/state_trackers/va/libvatracker.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/util/libmesautil.la \
	$(VA_LIBS) \
	$(GALLIUM_COMMON_LIB_DEPS)

if HAVE_GALLIUM_STATIC_TARGETS

TARGET_DRIVERS =
TARGET_CPPFLAGS =
TARGET_LIB_DEPS = $(top_builddir)/src/loader/libloader.la

include $(top_srcdir)/src/gallium/drivers/r600/Automake.inc
include $(top_srcdir)/src/gallium/drivers/radeonsi/Automake.inc

gallium_drv_video_la_SOURCES += target.c
gallium_drv_video_la_CPPFLAGS = $(TARGET_CPPFLAGS) -DGALLIUM_STATIC_TARGETS=1
gallium_drv_video_la_LIBADD += $(TARGET_LIB_DEPS) \
	$(TARGET_RADEON_WINSYS) $(TARGET_RADEON_COMMON)

else # HAVE_GALLIUM_STATIC_TARGETS

gallium_drv_video_la_CPPFLAGS = \
	$(GALLIUM_PIPE_LOADER_DEFINES) \
	-DPIPE_SEARCH_DIR=\"$(libdir)/gallium-pipe\"

gallium_drv_video_la_LIBADD += \
	$(top_builddir)/src/gallium/auxiliary/pipe-loader/libpipe_loader.la \
	$(GALLIUM_PIPE_LOADER_WINSYS_LIBS) \
	$(GALLIUM_PIPE_LOADER_LIBS)

endif # HAVE_GALLIUM_STATIC_TARGETS

if HAVE_MESA_LLVM
gallium_drv_video_la_LIBADD += $(LLVM_LIBS)
gallium_drv_video_la_LDFLAGS += $(LLVM_LDFLAGS)
endif
