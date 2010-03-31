# TODO Make core mesa more feature-aware and remove this file

include $(MESA)/sources.mak

LOCAL_ES1_INCLUDES :=			\
	-I.				\
	-I$(TOP)/src/mapi/es1api	\
	-I$(MESA)/state_tracker

LOCAL_ES2_INCLUDES := $(subst es1,es2, $(LOCAL_ES1_INCLUDES))

# MESA sources

VBO_OMITTED :=				\
	vbo/vbo_save.c			\
	vbo/vbo_save_api.c		\
	vbo/vbo_save_draw.c		\
	vbo/vbo_save_loopback.c
VBO_SOURCES := $(filter-out $(VBO_OMITTED), $(VBO_SOURCES))

SHADER_OMITTED :=			\
	shader/atifragshader.c
SHADER_SOURCES := $(filter-out $(SHADER_OMITTED), $(SHADER_SOURCES))

MESA_ES1_SOURCES :=			\
	$(MAIN_SOURCES)			\
	$(MATH_SOURCES)			\
	$(MATH_XFORM_SOURCES)		\
	$(VBO_SOURCES)			\
	$(TNL_SOURCES)			\
	$(SHADER_SOURCES)		\
	$(SWRAST_SOURCES)		\
	$(SWRAST_SETUP_SOURCES)		\
	$(COMMON_DRIVER_SOURCES)	\
	$(ASM_C_SOURCES)		\
	$(SLANG_SOURCES)

MESA_ES1_GALLIUM_SOURCES :=		\
	$(MAIN_SOURCES)			\
	$(MATH_SOURCES)			\
	$(VBO_SOURCES)			\
	$(STATETRACKER_SOURCES)		\
	$(SHADER_SOURCES)		\
	ppc/common_ppc.c		\
	x86/common_x86.c		\
	$(SLANG_SOURCES)

MESA_ES1_INCLUDES := $(INCLUDE_DIRS)

# right now es2 and es1 share MESA sources
MESA_ES2_SOURCES := $(MESA_ES1_SOURCES)
MESA_ES2_GALLIUM_SOURCES := $(MESA_ES1_GALLIUM_SOURCES)

MESA_ES2_INCLUDES := $(MESA_ES1_INCLUDES)

# asm is shared by any ES version and any library
MESA_ES_ASM := $(MESA_ASM_SOURCES)

# collect sources, adjust the pathes
ES1_SOURCES := $(addprefix $(MESA)/,$(MESA_ES1_SOURCES))
ES1_GALLIUM_SOURCES := $(addprefix $(MESA)/,$(MESA_ES1_GALLIUM_SOURCES))

ES2_SOURCES := $(addprefix $(MESA)/,$(MESA_ES2_SOURCES))
ES2_GALLIUM_SOURCES := $(addprefix $(MESA)/,$(MESA_ES2_GALLIUM_SOURCES))

# collect includes
ES1_INCLUDES := $(LOCAL_ES1_INCLUDES) $(MESA_ES1_INCLUDES)
ES2_INCLUDES := $(LOCAL_ES2_INCLUDES) $(MESA_ES2_INCLUDES)

# collect objects, including asm
ES1_OBJECTS :=					\
	$(MESA_ES1_SOURCES:.c=.o)		\
	$(MESA_ES_ASM:.S=.o)

ES1_GALLIUM_OBJECTS :=				\
	$(MESA_ES1_GALLIUM_SOURCES:.c=.o)	\
	$(MESA_ES_ASM:.S=.o)

ES2_OBJECTS :=					\
	$(MESA_ES2_SOURCES:.c=.o)		\
	$(MESA_ES_ASM:.S=.o)

ES2_GALLIUM_OBJECTS :=				\
	$(MESA_ES2_GALLIUM_SOURCES:.c=.o)	\
	$(MESA_ES_ASM:.S=.o)

# collect sources for makedepend
ES1_ALL_SOURCES := $(ES1_SOURCES) $(ES1_GALLIUM_SOURCES)
ES2_ALL_SOURCES := $(ES2_SOURCES) $(ES2_GALLIUM_SOURCES)

# sort to remove duplicates
ES1_ALL_SOURCES := $(sort $(ES1_ALL_SOURCES))
ES2_ALL_SOURCES := $(sort $(ES2_ALL_SOURCES))
