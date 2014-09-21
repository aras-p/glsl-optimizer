if HAVE_GALLIUM_SVGA

TARGET_DRIVERS += vmwgfx
TARGET_CPPFLAGS += -DGALLIUM_VMWGFX
TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/svga/drm/libsvgadrm.la \
	$(top_builddir)/src/gallium/drivers/svga/libsvga.la \
	$(LIBDRM_LIBS)

endif
