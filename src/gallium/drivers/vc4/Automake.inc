if HAVE_GALLIUM_VC4

TARGET_DRIVERS += vc4
TARGET_CPPFLAGS += -DGALLIUM_VC4
TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/vc4/drm/libvc4drm.la \
	$(top_builddir)/src/gallium/drivers/vc4/libvc4.la

if USE_VC4_SIMULATOR
TARGET_CPPFLAGS += -DUSE_VC4_SIMULATOR
endif

endif
