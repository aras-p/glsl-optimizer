if HAVE_GALLIUM_R600

TARGET_DRIVERS += r600
TARGET_CPPFLAGS += -DGALLIUM_R600
TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/drivers/r600/libr600.la \
	$(RADEON_LIBS) \
	$(LIBDRM_LIBS)

TARGET_RADEON_WINSYS ?= \
	$(top_builddir)/src/gallium/winsys/radeon/drm/libradeonwinsys.la

TARGET_RADEON_COMMON ?= \
	$(top_builddir)/src/gallium/drivers/radeon/libradeon.la

endif
