if HAVE_GALLIUM_NOUVEAU

TARGET_DRIVERS += nouveau
TARGET_CPPFLAGS += -DGALLIUM_NOUVEAU
TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/nouveau/drm/libnouveaudrm.la \
	$(top_builddir)/src/gallium/drivers/nouveau/libnouveau.la \
	$(NOUVEAU_LIBS) \
	$(LIBDRM_LIBS)

endif
