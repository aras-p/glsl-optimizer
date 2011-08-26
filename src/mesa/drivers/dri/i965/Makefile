TOP = ../../../../..
MESA_TOP := $(TOP)

# Import variables i965_*.
include Makefile.sources

include $(TOP)/configs/current

LIBNAME = i965_dri.so

include ../Makefile.defines

C_SOURCES := \
	$(i965_C_SOURCES) \
	$(COMMON_SOURCES)

CXX_SOURCES := $(i965_CXX_SOURCES)
ASM_SOURCES := $(i965_ASM_SOURCES)
DRIVER_DEFINES := $(addprefix -I, $(i965_INCLUDES))

INCLUDES += $(INTEL_CFLAGS)
DRI_LIB_DEPS += $(INTEL_LIBS)

include ../Makefile.targets

intel_decode.o: ../intel/intel_decode.c
intel_tex_layout.o: ../intel/intel_tex_layout.c
