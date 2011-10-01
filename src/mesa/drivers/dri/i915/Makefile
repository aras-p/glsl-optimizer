
TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = i915_dri.so

include ../Makefile.defines

# Import variables i915_*.
include Makefile.sources

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(i915_C_SOURCES)

ASM_SOURCES = 

DRIVER_DEFINES = $(addprefix -I$(TOP)/, $(i915_INCLUDES)) -DI915 \
	$(shell $(PKG_CONFIG) libdrm --atleast-version=2.3.1 \
				&& echo "-DDRM_VBLANK_FLIP=DRM_VBLANK_FLIP")

INCLUDES += $(INTEL_CFLAGS)
DRI_LIB_DEPS += $(INTEL_LIBS)

include ../Makefile.targets

intel_decode.o: ../intel/intel_decode.c

intel_tex_layout.o: ../intel/intel_tex_layout.c

