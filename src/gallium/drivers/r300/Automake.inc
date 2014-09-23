if HAVE_GALLIUM_R300

TARGET_DRIVERS += r300
TARGET_CPPFLAGS += -DGALLIUM_R300
TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/drivers/r300/libr300.la \
	$(RADEON_LIBS) \
	$(INTEL_LIBS)

TARGET_RADEON_WINSYS ?= \
	$(top_builddir)/src/gallium/winsys/radeon/drm/libradeonwinsys.la

endif
