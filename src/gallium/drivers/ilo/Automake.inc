if HAVE_GALLIUM_ILO

TARGET_DRIVERS += ilo
TARGET_CPPFLAGS += -DGALLIUM_ILO
TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/intel/drm/libintelwinsys.la \
	$(top_builddir)/src/gallium/drivers/ilo/libilo.la \
	$(INTEL_LIBS) \
	$(LIBDRM_LIBS)

endif
