if HAVE_GALLIUM_SOFTPIPE

TARGET_DRIVERS += swrast
TARGET_CPPFLAGS += -DGALLIUM_SOFTPIPE
TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/sw/dri/libswdri.la \
	$(top_builddir)/src/gallium/drivers/softpipe/libsoftpipe.la

if HAVE_DRI2
TARGET_DRIVERS += kms_swrast
TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/winsys/sw/kms-dri/libswkmsdri.la

endif
endif
