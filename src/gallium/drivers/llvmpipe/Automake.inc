if HAVE_GALLIUM_LLVMPIPE

TARGET_CPPFLAGS += -DGALLIUM_LLVMPIPE
TARGET_LIB_DEPS += \
	$(top_builddir)/src/gallium/drivers/llvmpipe/libllvmpipe.la

endif
