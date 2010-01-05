include $(MESA)/sources.mak

# LOCAL sources

LOCAL_ES1_SOURCES :=			\
	main/api_exec_es1.c		\
	main/get_es1.c			\
	main/specials_es1.c		\
	main/drawtex.c			\
	main/es_cpaltex.c		\
	main/es_enable.c		\
	main/es_fbo.c			\
	main/es_query_matrix.c		\
	main/es_texgen.c		\
	main/stubs.c			\
	glapi/glapi-es1/main/enums.c

LOCAL_ES1_GALLIUM_SOURCES :=		\
	$(LOCAL_ES1_SOURCES)		\
	state_tracker/st_cb_drawtex.c

# always use local version of GLAPI_ASM_SOURCES
LOCAL_ES1_API_ASM := $(addprefix glapi/glapi-es1/, $(GLAPI_ASM_SOURCES))

LOCAL_ES1_INCLUDES :=			\
	-I.				\
	-I./glapi/glapi-es1		\
	-I./state_tracker		\
	-I$(MESA)/state_tracker

LOCAL_ES2_SOURCES :=			\
	main/api_exec_es2.c		\
	main/get_es2.c			\
	main/specials_es2.c		\
	main/es_cpaltex.c		\
	main/es_fbo.c			\
	main/stubs.c			\
	glapi/glapi-es2/main/enums.c

LOCAL_ES2_GALLIUM_SOURCES :=		\
	$(LOCAL_ES2_SOURCES)

LOCAL_ES2_API_ASM := $(subst es1,es2, $(LOCAL_ES1_API_ASM))
LOCAL_ES2_INCLUDES := $(subst es1,es2, $(LOCAL_ES1_INCLUDES))

# MESA sources
# Ideally, the omit list should be replaced by features.

MAIN_OMITTED :=				\
	main/api_exec.c			\
	main/condrender.c		\
	main/dlopen.c			\
	main/enums.c			\
	main/get.c
MAIN_SOURCES := $(filter-out $(MAIN_OMITTED), $(MAIN_SOURCES))

VBO_OMITTED :=				\
	vbo/vbo_save.c			\
	vbo/vbo_save_api.c		\
	vbo/vbo_save_draw.c		\
	vbo/vbo_save_loopback.c
VBO_SOURCES := $(filter-out $(VBO_OMITTED), $(VBO_SOURCES))

STATETRACKER_OMITTED :=				\
	state_tracker/st_api.c			\
	state_tracker/st_cb_drawpixels.c	\
	state_tracker/st_cb_feedback.c		\
	state_tracker/st_cb_rasterpos.c		\
	state_tracker/st_draw_feedback.c
STATETRACKER_SOURCES := $(filter-out $(STATETRACKER_OMITTED), $(STATETRACKER_SOURCES))

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

MESA_ES1_API_SOURCES :=			\
	$(GLAPI_SOURCES)

MESA_ES1_INCLUDES := $(INCLUDE_DIRS)

# remove LOCAL sources from MESA sources
MESA_ES1_SOURCES := $(filter-out $(LOCAL_ES1_SOURCES), $(MESA_ES1_SOURCES))
MESA_ES1_GALLIUM_SOURCES := $(filter-out $(LOCAL_ES1_GALLIUM_SOURCES), $(MESA_ES1_GALLIUM_SOURCES))

# right now es2 and es1 share MESA sources
MESA_ES2_SOURCES := $(MESA_ES1_SOURCES)
MESA_ES2_GALLIUM_SOURCES := $(MESA_ES1_GALLIUM_SOURCES)
MESA_ES2_API_SOURCES := $(MESA_ES1_API_SOURCES)

MESA_ES2_INCLUDES := $(MESA_ES1_INCLUDES)

# asm is shared by any ES version and any library
MESA_ES_ASM := $(MESA_ASM_SOURCES)

# collect sources, adjust the pathes
ES1_SOURCES := $(LOCAL_ES1_SOURCES) $(addprefix $(MESA)/,$(MESA_ES1_SOURCES))
ES1_GALLIUM_SOURCES := $(LOCAL_ES1_GALLIUM_SOURCES) $(addprefix $(MESA)/,$(MESA_ES1_GALLIUM_SOURCES))
ES1_API_SOURCES := $(addprefix $(MESA)/,$(MESA_ES1_API_SOURCES))

ES2_SOURCES := $(LOCAL_ES2_SOURCES) $(addprefix $(MESA)/,$(MESA_ES2_SOURCES))
ES2_GALLIUM_SOURCES := $(LOCAL_ES2_GALLIUM_SOURCES) $(addprefix $(MESA)/,$(MESA_ES2_GALLIUM_SOURCES))
ES2_API_SOURCES := $(addprefix $(MESA)/,$(MESA_ES2_API_SOURCES))

# collect includes
ES1_INCLUDES := $(LOCAL_ES1_INCLUDES) $(MESA_ES1_INCLUDES)
ES2_INCLUDES := $(LOCAL_ES2_INCLUDES) $(MESA_ES2_INCLUDES)

# collect objects, including asm
ES1_OBJECTS :=					\
	$(LOCAL_ES1_SOURCES:.c=.o)		\
	$(MESA_ES1_SOURCES:.c=.o)		\
	$(MESA_ES_ASM:.S=.o)

ES1_GALLIUM_OBJECTS :=				\
	$(LOCAL_ES1_GALLIUM_SOURCES:.c=.o)	\
	$(MESA_ES1_GALLIUM_SOURCES:.c=.o)	\
	$(MESA_ES_ASM:.S=.o)

ES1_API_OBJECTS :=				\
	$(LOCAL_ES1_API_ASM:.S=.o)		\
	$(MESA_ES1_API_SOURCES:.c=.o)

ES2_OBJECTS :=					\
	$(LOCAL_ES2_SOURCES:.c=.o)		\
	$(MESA_ES2_SOURCES:.c=.o)		\
	$(MESA_ES_ASM:.S=.o)

ES2_GALLIUM_OBJECTS :=				\
	$(LOCAL_ES2_GALLIUM_SOURCES:.c=.o)	\
	$(MESA_ES2_GALLIUM_SOURCES:.c=.o)	\
	$(MESA_ES_ASM:.S=.o)

ES2_API_OBJECTS :=				\
	$(LOCAL_ES2_API_ASM:.S=.o)		\
	$(MESA_ES2_API_SOURCES:.c=.o)

# collect sources for makedepend
ES1_ALL_SOURCES := $(ES1_SOURCES) $(ES1_GALLIUM_SOURCES) $(ES1_API_SOURCES)
ES2_ALL_SOURCES := $(ES2_SOURCES) $(ES2_GALLIUM_SOURCES) $(ES2_API_SOURCES)

# sort to remove duplicates
ES1_ALL_SOURCES := $(sort $(ES1_ALL_SOURCES))
ES2_ALL_SOURCES := $(sort $(ES2_ALL_SOURCES))
