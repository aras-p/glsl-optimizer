if HAVE_GALLIUM_I915

TARGET_DRIVERS += i915
TARGET_CPPFLAGS += -DGALLIUM_I915
TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/i915/drm/libi915drm.la \
	$(top_builddir)/src/gallium/drivers/i915/libi915.la \
	$(INTEL_LIBS) \
	$(LIBDRM_LIBS)

endif
