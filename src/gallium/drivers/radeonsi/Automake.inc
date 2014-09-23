if HAVE_GALLIUM_RADEONSI

TARGET_DRIVERS += radeonsi
TARGET_CPPFLAGS += -DGALLIUM_RADEONSI
TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/drivers/radeonsi/libradeonsi.la \
	$(RADEON_LIBS) \
	$(LIBDRM_LIBS)

TARGET_RADEON_WINSYS ?= \
	$(top_builddir)/src/gallium/winsys/radeon/drm/libradeonwinsys.la

TARGET_RADEON_COMMON ?= \
	$(top_builddir)/src/gallium/drivers/radeon/libradeon.la

endif
