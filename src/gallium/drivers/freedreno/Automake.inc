if HAVE_GALLIUM_FREEDRENO

TARGET_DRIVERS += msm kgsl
TARGET_CPPFLAGS += -DGALLIUM_FREEDRENO
TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/freedreno/drm/libfreedrenodrm.la \
	$(top_builddir)/src/gallium/drivers/freedreno/libfreedreno.la \
	$(FREEDRENO_LIBS) \
	$(LIBDRM_LIBS)

endif
